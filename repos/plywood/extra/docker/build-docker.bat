@echo off
pushd
cd /D "%~dp0"
cd ..\..\..\..
mkdir data\docker\plywood
copy repos\plywood\extra\docker\Dockerfile.src data\docker\Dockerfile
copy repos\plywood\extra\docker\Dockerfile-deploy.src data\docker\Dockerfile-deploy
copy repos\plywood\extra\docker\deploy.sh.src data\docker\deploy.sh
git checkout-index -a -f --prefix=data\docker\plywood\
if %errorlevel% neq 0 exit /b %errorlevel%
cd data\docker
docker build -t plywood-docs .
if %errorlevel% neq 0 exit /b %errorlevel%
docker run --rm -v "/var/run/docker.sock:/var/run/docker.sock" -it plywood-docs ./deploy.sh
if %errorlevel% neq 0 exit /b %errorlevel%
popd
