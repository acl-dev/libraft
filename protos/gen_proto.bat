
cd ../src
mkdir proto_gen
cd proto_gen
for /R %%s in (*.pb.*) do (
echo %%s
rm %%s
) 
cd ../../
cd proto

for /R %%s in (*.proto) do (
echo %%s
protoc --proto_path="%cd%" --cpp_out="%cd%" "%%s"
) 


for /R %%s in (*.pb.*) do (
echo %%s
copy %%s ..\src\proto_gen
) 

