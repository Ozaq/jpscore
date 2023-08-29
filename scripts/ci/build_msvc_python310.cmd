call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
call C:\Python\Python310\python.exe -m venv venv310
call venv310\Scripts\activate.bat
pip install -r requirements.txt
mkdir build
set PYTHONPATH=%cd%\python_modules\jupedsim;%cd%\build\lib\Release
cd build
echo %PYTHONPATH%
cmake .. -DBUILD_TESTS=ON
cmake --build . --config Release
cmake --build . --config Release --target unittests
cd ..
python -m build -w