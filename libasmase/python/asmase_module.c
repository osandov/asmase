#include <Python.h>

#include <libasmase/libasmase.h>

#ifdef ASMASE_HAVE_FLOAT80
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#define PY_ARRAY_UNIQUE_SYMBOL asmase_ARRAY_API
#include <numpy/arrayobject.h>
#endif

extern PyObject *AssemblerDiagnostic;
extern PyTypeObject Assembler_type, Instance_type, RegisterValue_type;
extern PyStructSequence_Desc RegisterValue_desc;

static struct PyModuleDef asmasemodule = {
	PyModuleDef_HEAD_INIT,
	"asmase",
	"Interactive assembly language library",
	-1,
};

PyMODINIT_FUNC
PyInit_asmase(void)
{
	PyObject *m;

	if (libasmase_init() == -1)
		return PyErr_SetFromErrno(PyExc_OSError);

	Assembler_type.tp_new = PyType_GenericNew;
	if (PyType_Ready(&Assembler_type) < 0)
		return NULL;

	Instance_type.tp_new = PyType_GenericNew;
	if (PyType_Ready(&Instance_type) < 0)
		return NULL;

	m = PyModule_Create(&asmasemodule);
	if (m == NULL)
		return NULL;

	PyModule_AddIntMacro(m, ASMASE_REGISTERS_PROGRAM_COUNTER);
	PyModule_AddIntMacro(m, ASMASE_REGISTERS_SEGMENT);
	PyModule_AddIntMacro(m, ASMASE_REGISTERS_GENERAL_PURPOSE);
	PyModule_AddIntMacro(m, ASMASE_REGISTERS_STATUS);
	PyModule_AddIntMacro(m, ASMASE_REGISTERS_FLOATING_POINT);
	PyModule_AddIntMacro(m, ASMASE_REGISTERS_FLOATING_POINT_STATUS);
	PyModule_AddIntMacro(m, ASMASE_REGISTERS_VECTOR);
	PyModule_AddIntMacro(m, ASMASE_REGISTERS_VECTOR_STATUS);

	PyModule_AddIntMacro(m, ASMASE_REGISTER_U8);
	PyModule_AddIntMacro(m, ASMASE_REGISTER_U16);
	PyModule_AddIntMacro(m, ASMASE_REGISTER_U32);
	PyModule_AddIntMacro(m, ASMASE_REGISTER_U64);
	PyModule_AddIntMacro(m, ASMASE_REGISTER_U128);
	PyModule_AddIntMacro(m, ASMASE_REGISTER_FLOAT80);

	PyModule_AddIntMacro(m, ASMASE_SANDBOX_FDS);

	AssemblerDiagnostic = PyErr_NewException("asmase.AssemblerDiagnostic",
						 NULL, NULL);
	Py_INCREF(AssemblerDiagnostic);
	PyModule_AddObject(m, "AssemblerDiagnostic", AssemblerDiagnostic);

	Py_INCREF(&Assembler_type);
	PyModule_AddObject(m, "Assembler", (PyObject *)&Assembler_type);

	Py_INCREF(&Instance_type);
	PyModule_AddObject(m, "Instance", (PyObject *)&Instance_type);

	if (PyStructSequence_InitType2(&RegisterValue_type, &RegisterValue_desc) < 0)
		return NULL;
	Py_INCREF(&RegisterValue_type);
	PyModule_AddObject(m, "RegisterValue", (PyObject *)&RegisterValue_type);

#ifdef ASMASE_HAVE_FLOAT80
	import_array();
#endif

	return m;
}
