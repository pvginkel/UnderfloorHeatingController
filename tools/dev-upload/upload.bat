@echo off

setlocal enableextensions enabledelayedexpansion

pushd "%~dp0"

cd ..\..

docker build ^
    --tag underfloor-heating-controller-uploader ^
    . ^
    --file tools/dev-upload/Dockerfile 

echo "%~dp0"

docker run ^
    --rm ^
    -v %CD%/build/underfloor-heating-controller.bin:/workspace/app/underfloor-heating-controller-ota.bin ^
    -v %CD%/../HelmCharts/assets:/workspace/keys ^
    --add-host iotsupport.home:192.168.178.62 ^
    underfloor-heating-controller-uploader ^
    /workspace/keys/kubernetes-signing-key ^
    /workspace/app/underfloor-heating-controller-ota.bin ^
    iotsupport.home

popd
