import setuptools

setuptools.setup(
    name="petace-setops",
    version="0.2.0",
    author="Tiktok PILab",
    author_email="",
    description="petace-setops",
    url="",
    install_requires=[
    ],
    package_dir={"petace.setops": "./petace/setops/"},
    package_data={
        "petace.setops": ["*.so"],
    },
    classifiers=[
        "Programming Language :: Python :: 3",
    ],
    packages=setuptools.find_packages(exclude=["petace.tests"]),
    python_requires="==3.9.*",
)
