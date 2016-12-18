#include <Python.h>

#include <libasmase/libasmase.h>

typedef struct {
	PyObject_HEAD
	struct asmase_instance *a;
} Instance;

static int Instance_init(Instance *self)
{
	self->a = asmase_create_instance();
	if (!self->a) {
		PyErr_SetFromErrno(PyExc_OSError);
		return -1;
	}
	return 0;
}

static void Instance_dealloc(Instance *self)
{
	asmase_destroy_instance(self->a);
	Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *Instance_execute_code(Instance *self, PyObject *arg)
{
	Py_buffer code;
	int wstatus;
	int ret;

	ret = PyObject_GetBuffer(arg, &code, PyBUF_SIMPLE);
	if (ret == -1)
		return NULL;

	ret = asmase_execute_code(self->a, code.buf, code.len, &wstatus);
	PyBuffer_Release(&code);
	if (ret == -1)
		return PyErr_SetFromErrno(PyExc_OSError);
	return PyLong_FromLong(wstatus);
}

static PyObject *Instance_read_memory(Instance *self, PyObject *args,
				      PyObject *kwds)
{
	static char *keywords[] = {"addr", "len", NULL};
	PyObject *bytes;
	PyObject *pyaddr, *pylen;
	void *addr;
	size_t len;
	ssize_t ret;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!O!", keywords,
					 &PyLong_Type, &pyaddr,
					 &PyLong_Type, &pylen))
		return NULL;

	addr = PyLong_AsVoidPtr(pyaddr);
	len = PyLong_AsSize_t(pylen);
	if (PyErr_Occurred())
		return NULL;

	bytes = PyBytes_FromStringAndSize(NULL, len);
	if (!bytes)
		return NULL;

	ret = asmase_read_memory(self->a, PyBytes_AS_STRING(bytes), addr, len);
	if (ret == -1) {
		Py_DECREF(bytes);
		return PyErr_SetFromErrno(PyExc_OSError);
	}

	_PyBytes_Resize(&bytes, ret);
	return bytes;
}

static PyMethodDef Instance_methods[] = {
	{"execute_code", (PyCFunction)Instance_execute_code, METH_O,
	 "execute_code(code) -> int\n\n"
	 "Execute machine code on this instance. Returns the status of the\n"
	 "instance as returned by os.waitpid().\n\n"
	 "Arguments:\n"
	 "code -- machine code"},
	{"read_memory", (PyCFunction)Instance_read_memory,
	 METH_VARARGS | METH_KEYWORDS,
	 "read_memory(addr, len) -> bytes\n\n"
	 "Read a range of memory from this instance.\n\n"
	 "Keyword arguments:\n"
	 "addr -- memory address to read from\n"
	 "len -- amount of memory to read"},
	{}
};

#define INSTANCE_DOCSTRING	\
	"Create a new asmase instance. An instance can execute arbitrary code\n"	\
	"and have its state examined."

PyTypeObject Instance_type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"asmase.Instance",		/* tp_name */
	sizeof(Instance),		/* tp_basicsize */
	0,				/* tp_itemsize */
	(destructor)Instance_dealloc,	/* tp_dealloc */
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
	INSTANCE_DOCSTRING,		/* tp_doc */
	NULL,				/* tp_traverse */
	NULL,				/* tp_clear */
	NULL,				/* tp_richcompare */
	0,				/* tp_weaklistoffset */
	NULL,				/* tp_iter */
	NULL,				/* tp_iternext */
	Instance_methods,		/* tp_methods */
	NULL,				/* tp_members */
	NULL,				/* tp_getset */
	NULL,				/* tp_base */
	NULL,				/* tp_dict */
	NULL,				/* tp_descr_get */
	NULL,				/* tp_descr_set */
	0,				/* tp_dictoffset */
	(initproc)Instance_init,	/* tp_init */
};
