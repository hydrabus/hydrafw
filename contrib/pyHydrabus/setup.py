# Copyright 2019 Nicolas OBERLI
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import setuptools

import pyHydrabus

with open("README.md", "r") as fh:
    long_description = fh.read()

setuptools.setup(
    name="pyHydrabus",
    version=pyHydrabus.__version__,
    author="Baldanos",
    author_email="balda@balda.ch",
    description="Hydrabus BBIO python bindings",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/hydrabus/hydrafw",
    packages=setuptools.find_packages(),
    install_requires=['pyserial'],
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: Apache Software License",
        "Operating System :: OS Independent",
    ],
)
