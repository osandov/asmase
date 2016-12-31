#include <errno.h>
#include <Python.h>

#include <libasmase/libasmase.h>

#ifdef ASMASE_HAVE_FLOAT80
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#define NO_IMPORT_ARRAY
#define PY_ARRAY_UNIQUE_SYMBOL asmase_ARRAY_API
#include <numpy/arrayobject.h>
#endif

typedef struct {
	PyObject_HEAD
	struct asmase_instance *a;
} Instance;

static PyStructSequence_Field RegisterValue_fields[] = {
	{"value", "int or numpy float"},
	{"type", "type of value (asmase.ASMASE_REGISTER_*)"},
	{"set", "register set (asmase.ASMASE_REGISTERS_*)"},
	{"bits", "list of decoded status bits as strings"},
	{}
};
PyStructSequence_Desc RegisterValue_desc = {
	"asmase.RegisterValue",
	"Value of a register",
	RegisterValue_fields,
	4
};
PyTypeObject RegisterValue_type;

static int Instance_init(Instance *self, PyObject *args, PyObject *kwds)
{
	static char *keywords[] = {"flags", NULL};
	int flags = 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "|i", keywords, &flags))
		return -1;

	self->a = asmase_create_instance(flags);
	if (!self->a) {
		PyErr_SetFromErrno(PyExc_OSError);
		return -1;
	}
	return 0;
}

static void Instance_dealloc(Instance *self)
{
	if (self->a)
		asmase_destroy_instance(self->a);
	Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *PyErr_InstanceDestroyed(void)
{
	errno = ECHILD;
	return PyErr_SetFromErrno(PyExc_OSError);
}

static PyObject *Instance_destroy(Instance *self)
{
	if (!self->a)
		return PyErr_InstanceDestroyed();

	asmase_destroy_instance(self->a);
	self->a = NULL;
	Py_RETURN_NONE;
}

static PyObject *Instance_getpid(Instance *self, PyObject *arg)
{
	if (!self->a)
		return PyErr_InstanceDestroyed();

	return PyLong_FromLong(asmase_getpid(self->a));
}

static PyObject *Instance_execute_code(Instance *self, PyObject *arg)
{
	Py_buffer code;
	int wstatus;
	int ret;

	if (!self->a)
		return PyErr_InstanceDestroyed();

	ret = PyObject_GetBuffer(arg, &code, PyBUF_SIMPLE);
	if (ret == -1)
		return NULL;

	ret = asmase_execute_code(self->a, code.buf, code.len, &wstatus);
	PyBuffer_Release(&code);
	if (ret == -1)
		return PyErr_SetFromErrno(PyExc_OSError);
	return PyLong_FromLong(wstatus);
}

static PyObject *Instance_get_register_sets(Instance *self)
{
	if (!self->a)
		return PyErr_InstanceDestroyed();

	return PyLong_FromLong(asmase_get_register_sets(self->a));
}

static PyObject *PyLong_FromUnsignedInt128(uint64_t lo, uint64_t hi)
{
	PyObject *pylo = NULL, *pyhi = NULL;
	PyObject *shift = NULL, *shifted = NULL;
	PyObject *result = NULL;

	pylo = PyLong_FromUnsignedLongLong(lo);
	if (!pylo)
		goto out;

	pyhi = PyLong_FromUnsignedLongLong(hi);
	if (!pyhi)
		goto out;

	shift = PyLong_FromUnsignedLong(64);
	if (!shift)
		goto out;

	shifted = PyNumber_InPlaceLshift(pyhi, shift);
	if (!shifted)
		goto out;

	result = PyNumber_InPlaceAdd(shifted, pylo);
out:
	Py_XDECREF(shifted);
	Py_XDECREF(shift);
	Py_XDECREF(pyhi);
	Py_XDECREF(pylo);
	return result;
}

#ifdef ASMASE_HAVE_FLOAT80
static PyObject *PyLongDoubleArrType_FromLongDouble(long double value)
{
	PyArray_Descr *dtype = NULL;
	PyObject *itemsize = NULL;
	PyObject *result = NULL;

	dtype = PyArray_DescrFromType(NPY_LONGDOUBLE);
	if (!dtype)
		goto out;

	itemsize = PyLong_FromSize_t(sizeof(value));
	if (!itemsize)
		goto out;

	result = PyArray_Scalar(&value, dtype, itemsize);
out:
	Py_XDECREF(itemsize);
	Py_XDECREF(dtype);
	return result;
}
#endif

static PyObject *get_register_value(struct asmase_register *reg)
{
	PyObject *value = NULL;

	switch (reg->type) {
	case ASMASE_REGISTER_U8:
		value = PyLong_FromUnsignedLong(reg->u8);
		break;
	case ASMASE_REGISTER_U16:
		value = PyLong_FromUnsignedLong(reg->u16);
		break;
	case ASMASE_REGISTER_U32:
		value = PyLong_FromUnsignedLong(reg->u32);
		break;
	case ASMASE_REGISTER_U64:
		value = PyLong_FromUnsignedLongLong(reg->u64);
		break;
	case ASMASE_REGISTER_U128:
		value = PyLong_FromUnsignedInt128(reg->u128_lo, reg->u128_hi);
		break;
#ifdef ASMASE_HAVE_FLOAT80
	case ASMASE_REGISTER_FLOAT80:
		value = PyLongDoubleArrType_FromLongDouble(reg->float80);
		break;
#endif
	default:
		PyErr_SetString(PyExc_RuntimeError,
				"invalid register type encountered");
		break;
	}

	return value;
}

static PyObject *get_register_status_bits(struct asmase_register *reg)
{
	PyObject *list;
	uintmax_t value;
	size_t i;
	int ret;

	if (!reg->num_status_bits)
		Py_RETURN_NONE;

	switch (reg->type) {
	case ASMASE_REGISTER_U8:
		value = reg->u8;
		break;
	case ASMASE_REGISTER_U16:
		value = reg->u16;
		break;
	case ASMASE_REGISTER_U32:
		value = reg->u32;
		break;
	case ASMASE_REGISTER_U64:
		value = reg->u64;
		break;
	default:
		PyErr_SetString(PyExc_RuntimeError,
				"status register bits on unexpected type");
		return NULL;
	}

	list = PyList_New(0);
	if (!list)
		return NULL;

	for (i = 0; i < reg->num_status_bits; i++) {
		PyObject *str;
		char *flag;

		flag = asmase_status_register_format(&reg->status_bits[i],
						     value);
		if (!flag)
			goto err;
		if (!*flag) {
			free(flag);
			continue;
		}

		str = PyUnicode_FromString(flag);
		free(flag);
		if (!str)
			goto err;

		ret = PyList_Append(list, str);
		Py_DECREF(str);
		if (ret == -1)
			goto err;
	}

	return list;
err:
	Py_DECREF(list);
	return NULL;
}

static PyObject *BuildRegisterValue(PyObject *value, PyObject *type,
				    PyObject *set, PyObject *status_bits)
{
	PyObject *register_value;

	register_value = PyStructSequence_New(&RegisterValue_type);
	if (!register_value)
		return NULL;

	Py_INCREF(value);
	PyStructSequence_SetItem(register_value, 0, value);
	Py_INCREF(type);
	PyStructSequence_SetItem(register_value, 1, type);
	Py_INCREF(set);
	PyStructSequence_SetItem(register_value, 2, set);
	Py_INCREF(status_bits);
	PyStructSequence_SetItem(register_value, 3, status_bits);
	return register_value;
}

static int add_register(PyObject *odict, struct asmase_register *reg)
{
	PyObject *name = NULL;
	PyObject *value = NULL, *type = NULL, *set = NULL, *status_bits = NULL;
	PyObject *register_value = NULL;
	int ret = -1;

	name = PyUnicode_FromString(reg->name);
	if (!name)
		goto out;

	value = get_register_value(reg);
	if (!value)
		goto out;

	type = PyLong_FromLong(reg->type);
	if (!type)
		goto out;

	set = PyLong_FromLong(reg->set);
	if (!set)
		goto out;

	status_bits = get_register_status_bits(reg);
	if (!status_bits)
		goto out;

	register_value = BuildRegisterValue(value, type, set, status_bits);
	if (!register_value)
		goto out;

	ret = PyODict_SetItem(odict, name, register_value);
out:
	Py_XDECREF(register_value);
	Py_XDECREF(status_bits);
	Py_XDECREF(set);
	Py_XDECREF(type);
	Py_XDECREF(value);
	Py_XDECREF(name);
	return ret;
}

static PyObject *Instance_get_registers(Instance *self, PyObject *args,
					PyObject *kwds)
{
	static char *keywords[] = {"regsets", NULL};
	PyObject *odict = NULL;
	struct asmase_register *regs;
	size_t num_regs, i;
	int regsets;
	int ret;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", keywords, &regsets))
		return NULL;

	if (!self->a)
		return PyErr_InstanceDestroyed();

	ret = asmase_get_registers(self->a, regsets, &regs, &num_regs);
	if (ret == -1)
		return PyErr_SetFromErrno(PyExc_OSError);

	odict = PyODict_New();
	if (!odict)
		goto err;

	for (i = 0; i < num_regs; i++) {
		ret = add_register(odict, &regs[i]);
		if (ret == -1)
			goto err;
	}
	free(regs);

	return odict;
err:
	Py_XDECREF(odict);
	free(regs);
	return NULL;
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

	if (!self->a)
		return PyErr_InstanceDestroyed();

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

static PyObject *Instance_enter(Instance *self)
{
	Py_INCREF(self);
	return (PyObject *)self;
}

static PyObject *Instance_exit(Instance *self, PyObject *args)
{
	return Instance_destroy(self);
}

static PyMethodDef Instance_methods[] = {
	{"destroy", (PyCFunction)Instance_destroy, METH_NOARGS,
	 "destroy()\n\n"
	 "Destroy this instance. No other methods must be called on this\n"
	 "instance after this."},
	{"getpid", (PyCFunction)Instance_getpid, METH_NOARGS,
	 "getpid() -> int\n\n"
	 "Get the PID of this instance."},
	{"execute_code", (PyCFunction)Instance_execute_code, METH_O,
	 "execute_code(code) -> int\n\n"
	 "Execute machine code on this instance. Returns the status of the\n"
	 "instance as returned by os.waitpid().\n\n"
	 "Arguments:\n"
	 "code -- machine code"},
	{"get_register_sets", (PyCFunction)Instance_get_register_sets,
	 METH_NOARGS,
	 "get_register_sets() -> int\n\n"
	 "Get the types of registers available on this instance. Returns a\n"
	 "bitmask of ASMASE_REGISTERS_*."},
	{"get_registers", (PyCFunction)Instance_get_registers,
	 METH_VARARGS | METH_KEYWORDS,
	 "get_registers(regsets) -> OrderedDict of str: RegisterValue\n\n"
	 "Get the values of the registers of this instance. Returns a mapping\n"
	 "from register name to RegisterValue.\n\n"
	 "Arguments:\n"
	 "regsets -- bitmask of ASMASE_REGISTERS_* representing which\n"
	 "registers to get"},
	{"read_memory", (PyCFunction)Instance_read_memory,
	 METH_VARARGS | METH_KEYWORDS,
	 "read_memory(addr, len) -> bytes\n\n"
	 "Read a range of memory from this instance.\n\n"
	 "Arguments:\n"
	 "addr -- memory address to read from\n"
	 "len -- amount of memory to read"},
	{"__enter__", (PyCFunction)Instance_enter, METH_NOARGS},
	{"__exit__", (PyCFunction)Instance_exit, METH_VARARGS},
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
