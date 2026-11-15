param (
    [Parameter(Mandatory=$true)]
    [string]$ImageTag
)

$EnvFile = Join-Path $PSScriptRoot "..\tools\.env.store"
if (Test-Path $EnvFile) {
    Get-Content $EnvFile | ForEach-Object {
        if ($_ -match "^\s*([^#][^=]+)=(.*)$") {
            [System.Environment]::SetEnvironmentVariable($matches[1].Trim(), $matches[2].Trim(), "Process")
        }
    }
}

$RedisUrl = $env:REDIS_URL
if (-not $RedisUrl) {
    throw "REDIS_URL is not set. Please set REDIS_URL in tools/.env.store or environment variables."
}

$AcrRegistry = if ($env:ACR_REGISTRY) { $env:ACR_REGISTRY } else { "your-registry.azurecr.io" }

$vmNames = az vm list -g CHAT --query "[].name" -o tsv
foreach ($vmName in $vmNames) {
    $vmName = $vmName.Trim()
    if (-not $vmName) { continue }
    $hostName = "${vmName}-2026.japaneast.cloudapp.azure.com"
    
    # We pass the env variables to docker run instead of positional arguments
    $script = "sudo docker stop `$(`sudo docker ps -q); sudo docker rm `$(`sudo docker ps -aq); sudo docker run -d --net host --restart always -e REDIS_URL='$RedisUrl' -e ENDPOINT='${hostName}:4000' ${AcrRegistry}/chat-server:$ImageTag"
    
    Write-Host "Deploying $ImageTag to $vmName..."
    az vm run-command invoke -g CHAT -n $vmName --command-id RunShellScript --scripts $script --no-wait
}
Write-Host "Deployment initiated for all active VMs ($ImageTag)."
