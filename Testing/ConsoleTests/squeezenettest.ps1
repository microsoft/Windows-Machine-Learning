
$imagePath = join-path @(Get-Location) "kitten_224.png"
Start-Process -FilePath ".\SqueezeNetObjectDetectionCPP.exe" -ArgumentList $imagePath -RedirectStandardOutput ".\squeezenetoutput.txt" -Wait

$output = Get-Content .\squeezenetoutput.txt
$expected = Get-Content .\squeezenetexpected.txt

# compare the probability scores at the end of the output
if (@(Compare-Object $output[6..$output.Count] $expected -SyncWindow 0).Length -ne 0) {
    throw "squeezenet output is not expected"
}
echo "squeezenetcpp predicted expected labels"