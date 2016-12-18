#include <Python.h>

#include <libasmase/libasmase.h>

extern PyObject *AssemblerDiagnostic;
extern PyTypeObject Assembler_type, Instance_type;

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

	AssemblerDiagnostic = PyErr_NewException("asmase.AssemblerDiagnostic",
						 NULL, NULL);
	Py_INCREF(AssemblerDiagnostic);
	PyModule_AddObject(m, "AssemblerDiagnostic", AssemblerDiagnostic);

	Py_INCREF(&Assembler_type);
	PyModule_AddObject(m, "Assembler", (PyObject *)&Assembler_type);

	Py_INCREF(&Instance_type);
	PyModule_AddObject(m, "Instance", (PyObject *)&Instance_type);

	return m;
}
