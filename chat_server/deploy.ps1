param (
    [Parameter(Mandatory=$true)]
    [string]$ImageTag
)

$RedisUrl = "tcp://default:REDACTED_REDIS_PASSWORD@your-redis-host.example.com:6379"

$vms = "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z"
foreach ($vm in $vms) {
    $vmName = "chat-vm-$vm"
    $hostName = "chat-vm-$vm-2026.japaneast.cloudapp.azure.com"
    
    # We pass the env variables to docker run instead of positional arguments
    $script = "sudo docker stop `$(`sudo docker ps -q); sudo docker rm `$(`sudo docker ps -aq); sudo docker run -d --net host --restart always -e REDIS_URL='$RedisUrl' -e ENDPOINT='${hostName}:4000' your-registry.azurecr.io/chat-server:$ImageTag"
    
    Write-Host "Deploying $ImageTag to $vmName..."
    az vm run-command invoke -g CHAT -n $vmName --command-id RunShellScript --scripts $script --no-wait
}
Write-Host "Deployment initiated for all 26 VMs ($ImageTag)."
