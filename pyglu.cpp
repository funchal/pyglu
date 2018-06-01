#include <Python.h>
#include <iostream>

struct borrow {};
struct steal {};

// a wrapper around python objects with automatic reference counting
struct object {
    // empty constructor
    constexpr object() noexcept : m_ptr(nullptr) {};

    // borrowing constructor, increments refcount
    object(PyObject* ptr, borrow) : m_ptr(ptr) { Py_XINCREF(m_ptr); }

    // stealing constructor, does not increment refcount
    object(PyObject* ptr, steal) noexcept : m_ptr(ptr) {}

    // copy constructor, increments refcount
    object(const object& o) : object(o.get(), borrow{}) {}

    // move constructor, steals
    object(object&& o) noexcept : object(o.release(), steal{}) {}

    // copy assignment, increments refcount
    object& operator=(const object& o) {
        object tmp(o);
        using std::swap;
        swap(*this, tmp);
        return *this;
    }

    // move assignment, steals
    object& operator=(object&& o) noexcept {
        object tmp(std::move(o));
        using std::swap;
        swap(*this, tmp);
        return *this;
    }

    // steal, ie reset object and return raw handle, without modifying reference count
    // (named after std::unique_ptr::release)
    PyObject* release() noexcept {
        auto tmp = m_ptr;
        m_ptr = nullptr;
        return tmp;
    }

    ~object() { Py_XDECREF(m_ptr); }

    PyObject* get() const { return m_ptr; }

    friend void swap(object& a, object& b) {
        using std::swap;
        swap(a.m_ptr, b.m_ptr);
    }

protected:
    PyObject* m_ptr;
};

struct module : public object {
    module() {
        PyModuleDef* def = static_cast<PyModuleDef*>(PyObject_Malloc(sizeof(PyModuleDef)));
        if (!def) {
            std::cerr << "fail" << std::endl;
        }
        *def = {};
        def->m_base = PyModuleDef_HEAD_INIT;
        def->m_name = "example";
        def->m_doc = "";
        def->m_size = -1;
        // def->m_free = [](void*) { std::cout << "free" << std::endl; };
        m_ptr = PyModule_Create(def);
        if (!m_ptr) {
            std::cerr << "fail" << std::endl;
        }

        std::cout << Py_REFCNT(m_ptr) << std::endl;
    }
    // TODO:
    // use python 3.5 modules
    // def function submodules, object
    // globals, reload, import
};

struct pybool : public object {
    pybool() : object(Py_False, borrow{}) {}
    pybool(std::nullptr_t) : object() {}
    pybool(bool v) : object(v ? Py_True : Py_False, borrow{}) {}
    operator bool() const { return m_ptr == Py_True; }

    static pybool convert(const object& o) {
        if (o.get()) {
            auto value = PyObject_IsTrue(o.get());
            if (value == 1) {
                return pybool(true);
            } else if (value == 0) {
                return pybool(false);
            }
        }
        return pybool(nullptr);
    }
};

struct function : public object {
    // PyCFunction_Check
    function() : object() {}
    function(PyObject* o, borrow) : object(PyCallable_Check(o) ? o : nullptr, borrow{}) {}
    function(PyObject* o, steal) : object(PyCallable_Check(o) ? o : nullptr, steal{}) {}

    static function convert(const object& o) {
        if (o.get()) {
            if (PyInstanceMethod_Check(o.get())) {
                return function(PyInstanceMethod_GET_FUNCTION(o.get()), borrow{});
            } else if (PyMethod_Check(o.get())) {
                return function(PyMethod_GET_FUNCTION(o.get()), borrow{});
            }
        }
        return function();
    }
};

template<typename T>
struct function_record {
    T impl;
};

struct cfunction : public function {
    cfunction() = default;
    cfunction(void (*f)()) {
        auto impl = [f=std::move(f)]() {
            f();
        };

        auto rec = new function_record<decltype(impl)>{std::move(impl)};
    }

    // TODO:
    // name
};

#define EXPORT __attribute__ ((visibility("default")))

namespace {
    extern "C" EXPORT PyObject* PyInit_example() {
        // TODO: check python version
        std::cout << "compiled against " << PY_MAJOR_VERSION << "." << PY_MINOR_VERSION << std::endl;
        std::cout << "running with " << Py_GetVersion() << std::endl;

        auto a = module();
        return a.release();
    }
}