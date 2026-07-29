$vms = "v", "y", "z"

$jobs = @()

foreach ($l in $vms) {
    $vmName = "chat-vm-$l"
    $fqdn = "chat-vm-$l-2026.japaneast.cloudapp.azure.com"
    
    $scriptBlock = {
        param($vm, $fqdn)
        Set-Location "C:\Users\kenjinote\Desktop\chat"
        Write-Host "Updating $vm to v16..."
        az vm run-command invoke -g chat -n $vm --command-id RunShellScript --scripts "@update_docker.sh" --parameters $fqdn --subscription 00000000-0000-0000-0000-000000000000
        Write-Host "Finished $vm"
    }
    $jobs += Start-Job -ScriptBlock $scriptBlock -ArgumentList $vmName, $fqdn
}

Write-Host "Waiting for the 3 VMs to update..."
Wait-Job $jobs
Receive-Job $jobs
Remove-Job $jobs
Write-Host "All 3 VMs updated successfully!"
