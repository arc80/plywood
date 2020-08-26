pushd
cd /D "%~dp0"
cd ..\..\..\..
mkdir data\docker\plywood
copy repos\plywood\extra\docker\Dockerfile.src data\docker\Dockerfile
git checkout-index -a -f --prefix=data\docker\plywood\
cd data\docker
docker build -t plywood-docs .
rem docker run -p 127.0.0.1:8080:8080/tcp plywood-docs
