$letters = "abcdefghijklmnopqrstuvwxyz".ToCharArray()
$jobs = @()

foreach ($l in $letters) {
    $vmName = "chat-vm-$l"
    $fqdn = "chat-vm-$l-2026.japaneast.cloudapp.azure.com"
    
    $scriptBlock = {
        param($vm, $fqdn)
        Set-Location "C:\Users\kenjinote\Desktop\chat"
        az vm run-command invoke -g chat -n $vm --command-id RunShellScript --scripts "@serve_json.sh" --subscription 00000000-0000-0000-0000-000000000000
        az network nsg rule create -g chat --nsg-name "$vm`NSG" --name AllowHTTP --protocol Tcp --direction Inbound --priority 1010 --source-address-prefix "*" --source-port-range "*" --destination-address-prefix "*" --destination-port-range 80 --access Allow --subscription 00000000-0000-0000-0000-000000000000
    }
    $jobs += Start-Job -ScriptBlock $scriptBlock -ArgumentList $vmName, $fqdn
}

Wait-Job $jobs
Receive-Job $jobs
Remove-Job $jobs
