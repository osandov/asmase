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

static PyObject *Instance_get_register_sets(Instance *self)
{
	return PyLong_FromLong(asmase_get_register_sets(self->a));
}

/*
 * Returns a borrowed reference.
 */
static PyObject *get_regset_odict(PyObject *odict, int regset)
{
	PyObject *key = NULL;
	PyObject *regset_odict = NULL;
	int ret;

	key = PyLong_FromLong(regset);
	if (!key)
		goto out;

	regset_odict = PyODict_GetItem(odict, key);
	if (!regset_odict) {
		regset_odict = PyODict_New();
		if (!regset_odict)
			goto out;

		ret = PyODict_SetItem(odict, key, regset_odict);
		Py_DECREF(regset_odict);
		if (ret == -1)
			regset_odict = NULL;
	}

out:
	Py_XDECREF(key);
	return regset_odict;
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

static int add_register(PyObject *regset_odict, struct asmase_register *reg)
{
	PyObject *name = NULL;
	PyObject *value = NULL, *status_bits = NULL;
	PyObject *tuple = NULL;
	int ret = -1;

	name = PyUnicode_FromString(reg->name);
	if (!name)
		goto out;

	value = get_register_value(reg);
	if (!value)
		goto out;

	status_bits = get_register_status_bits(reg);
	if (!status_bits)
		goto out;

	tuple = Py_BuildValue("(OiO)", value, reg->type, status_bits);
	if (!tuple)
		goto out;

	ret = PyODict_SetItem(regset_odict, name, tuple);
out:
	Py_XDECREF(tuple);
	Py_XDECREF(status_bits);
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

	ret = asmase_get_registers(self->a, regsets, &regs, &num_regs);
	if (ret == -1)
		return PyErr_SetFromErrno(PyExc_OSError);

	odict = PyODict_New();
	if (!odict)
		goto err;

	for (i = 0; i < num_regs; i++) {
		PyObject *regset_odict;

		regset_odict = get_regset_odict(odict, regs[i].set);
		if (!regset_odict)
			goto err;

		ret = add_register(regset_odict, &regs[i]);
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
	{"get_register_sets", (PyCFunction)Instance_get_register_sets,
	 METH_NOARGS,
	 "get_register_sets() -> int\n\n"
	 "Get the types of registers available on this instance. Returns a\n"
	 "bitmask of ASMASE_REGISTERS_*."},
	{"get_registers", (PyCFunction)Instance_get_registers,
	 METH_VARARGS | METH_KEYWORDS,
	 "get_registers(regsets) -> register sets\n\n"
	 "Get the values of the registers of this instance. Returns a mapping\n"
	 "from register set (ASMASE_REGISTERS_*) to a mapping from register\n"
	 "name to (value, ASMASE_REGISTER_* type, decoded status bits).\n\n"
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
