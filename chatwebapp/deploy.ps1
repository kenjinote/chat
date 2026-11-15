param(
    [switch]$frontendOnly,
    [switch]$backendOnly
)

$EnvFile = Join-Path $PSScriptRoot "..\tools\.env.store"
if (Test-Path $EnvFile) {
    Get-Content $EnvFile | ForEach-Object {
        if ($_ -match "^\s*([^#][^=]+)=(.*)$") {
            [System.Environment]::SetEnvironmentVariable($matches[1].Trim(), $matches[2].Trim(), "Process")
        }
    }
}

$remoteUser = if ($env:WEBAPP_DEPLOY_USER) { $env:WEBAPP_DEPLOY_USER } else { "azureuser" }
$remoteHost = if ($env:WEBAPP_DEPLOY_HOST) { $env:WEBAPP_DEPLOY_HOST } else { "0.0.0.0" }
$remoteDistPath = "/home/${remoteUser}/dist"
$remoteServerPath = "/home/${remoteUser}/server"

Write-Host "Starting deployment to $remoteHost..." -ForegroundColor Cyan

if (-not $backendOnly) {
    Write-Host "`n--- Deploying Frontend ---" -ForegroundColor Yellow
    
    Write-Host "1/4 Building frontend..."
    npm run build
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Build failed. Aborting deployment."
        exit 1
    }
    
    Write-Host "2/4 Zipping dist folder..."
    if (Test-Path "deploy_dist.zip") {
        Remove-Item "deploy_dist.zip" -Force
    }
    Compress-Archive -Path dist\* -DestinationPath deploy_dist.zip -Force
    
    Write-Host "3/4 Uploading to server..."
    scp -o StrictHostKeyChecking=no deploy_dist.zip "${remoteUser}@${remoteHost}:~/"
    
    Write-Host "4/4 Extracting on server..."
    ssh -o StrictHostKeyChecking=no ${remoteUser}@${remoteHost} "sudo rm -rf ${remoteDistPath}/* && sudo unzip -o deploy_dist.zip -d ${remoteDistPath} && sudo chown -R ${remoteUser}:${remoteUser} ${remoteDistPath}"
    
    Write-Host "Cleaning up local zip..."
    Remove-Item "deploy_dist.zip" -Force
    
    Write-Host "Frontend deployed successfully!" -ForegroundColor Green
}

if (-not $frontendOnly) {
    Write-Host "`n--- Deploying Backend ---" -ForegroundColor Yellow
    
    Write-Host "1/2 Uploading server files..."
    scp -o StrictHostKeyChecking=no -r server/* "${remoteUser}@${remoteHost}:${remoteServerPath}/"
    
    Write-Host "2/2 Restarting PM2 process..."
    ssh -o StrictHostKeyChecking=no ${remoteUser}@${remoteHost} "pm2 restart chatapp"
    
    Write-Host "Backend deployed successfully!" -ForegroundColor Green
}

Write-Host "`nAll deployments complete!" -ForegroundColor Cyan
