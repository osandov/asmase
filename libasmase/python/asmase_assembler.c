#include <Python.h>

#include <libasmase/assembler.h>

typedef struct {
	PyObject_HEAD
	struct asmase_assembler *as;
} Assembler;

PyObject *AssemblerDiagnostic;

static int Assembler_init(Assembler *self)
{
	self->as = asmase_create_assembler();
	if (!self->as) {
		PyErr_SetFromErrno(PyExc_OSError);
		return -1;
	}
	return 0;
}

static void Assembler_dealloc(Assembler *self)
{
	asmase_destroy_assembler(self->as);
	Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *Assembler_assemble_code(Assembler *self, PyObject *args,
					 PyObject *kwds)
{
	static char *keywords[] = {
		"code", "filename", "line", NULL,
	};
	PyObject *obj = NULL, *str;
	const char *code, *filename = "";
	int line = 1;
	char *out;
	size_t len;
	int ret;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "s|si", keywords, &code,
					 &filename, &line))
		return NULL;

	ret = asmase_assemble_code(self->as, filename, line, code, &out, &len);
	switch (ret) {
	case 0:
		obj = PyBytes_FromStringAndSize(out, len);
		free(out);
		break;
	case 1:
		str = PyUnicode_FromStringAndSize(out, len);
		if (str) {
			PyErr_SetObject(AssemblerDiagnostic, str);
			Py_DECREF(str);
		}
		free(out);
		break;
	default:
		PyErr_SetString(PyExc_RuntimeError,
				"internal error while assembling");
		break;
	}
	return obj;
}

static PyMethodDef Assembler_methods[] = {
	{"assemble_code", (PyCFunction)Assembler_assemble_code,
	 METH_VARARGS | METH_KEYWORDS,
	 "assemble_code(code, filename='', line=1) -> bytes\n\n"
	 "Translate assembly code into machine code. Raises AssemblerDiagnostic\n"
	 "if there is a problem with the assembly code.\n\n"
	 "Keyword arguments:\n"
	 "code -- assembly code\n"
	 "filename -- filename for diagnostics\n"
	 "line -- line number for diagnostics\n"},
	{},
};

#define ASSEMBLER_DOCSTRING	\
	"Create a new assembler which translates assembly code for the running\n"	\
	"architecture into native machine code."

PyTypeObject Assembler_type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"asmase.Assembler",		/* tp_name */
	sizeof(Assembler),		/* tp_basicsize */
	0,				/* tp_itemsize */
	(destructor)Assembler_dealloc,	/* tp_dealloc */
	NULL,				/* tp_print */
	NULL,				/* tp_getattr */
	NULL,				/* tp_setattr */
	NULL,				/* tp_as_async */
	NULL,				/* tp_repr */
	NULL,				/* tp_as_number */
	NULL,				/* tp_as_sequence */
	NULL,				/* tp_as_mapping */
	NULL,				/* tp_hash  */
	NULL,				/* tp_call */
	NULL,				/* tp_str */
	NULL,				/* tp_getattro */
	NULL,				/* tp_setattro */
	NULL,				/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,		/* tp_flags */
	ASSEMBLER_DOCSTRING,		/* tp_doc */
	NULL,				/* tp_traverse */
	NULL,				/* tp_clear */
	NULL,				/* tp_richcompare */
	0,				/* tp_weaklistoffset */
	NULL,				/* tp_iter */
	NULL,				/* tp_iternext */
	Assembler_methods,		/* tp_methods */
	NULL,				/* tp_members */
	NULL,				/* tp_getset */
	NULL,				/* tp_base */
	NULL,				/* tp_dict */
	NULL,				/* tp_descr_get */
	NULL,				/* tp_descr_set */
	0,				/* tp_dictoffset */
	(initproc)Assembler_init,	/* tp_init */
};
