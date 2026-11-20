from setuptools import setup
from pybind11.setup_helpers import Pybind11Extension, build_ext


setup(
    name="grafos-bindings",
    version="0.1.0",
    description="Python bindings for the C++ graph algorithms",
    ext_modules=[
        Pybind11Extension(
            "grafos",
            ["src/python_bindings.cpp"],
            cxx_std=17,
        )
    ],
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
)
