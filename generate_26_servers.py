import json
import copy
import base64

def get_setup_script(letter):
    script = f"""#!/bin/bash
apt-get update
apt-get install -y docker.io
systemctl enable docker
systemctl start docker
docker login your-registry.azurecr.io -u your-registry -p REDACTED_ACR_PASSWORD
docker pull your-registry.azurecr.io/chat-server:v10
docker stop $(docker ps -q) || true
docker rm $(docker ps -aq) || true
docker run -d --net host --restart always your-registry.azurecr.io/chat-server:v10 ./chat_server chat-vm-{letter}-2026.japaneast.cloudapp.azure.com:4000 tcp://your-redis-host.example.com:6379
"""
    return base64.b64encode(script.encode('utf-8')).decode('utf-8')

with open('chat_sfu_template.json', 'r', encoding='utf-8') as f:
    template = json.load(f)

with open('params.json', 'r', encoding='utf-8') as f:
    params = json.load(f)

# Identify B resources to use as a clone template
b_templates = []
new_resources = []

for res in template['resources']:
    res_str = json.dumps(res)
    # Collect b resources as templates
    if 'chat_vm_b' in res_str or 'chat-vm-b' in res_str:
        b_templates.append(copy.deepcopy(res))
        continue # don't add to new_resources yet
        
    # Delete unused container instances
    if res['type'] == 'Microsoft.ContainerInstance/containerGroups':
        name_str = json.dumps(res['name'])
        if 'chat-server-a' in name_str or 'chat-server-b' in name_str or 'chat_server_a' in name_str or 'chat_server_b' in name_str:
            continue
            

    new_resources.append(copy.deepcopy(res))

# Generate b through z
letters = [chr(i) for i in range(ord('b'), ord('z')+1)]

for letter in letters:
    for b_res in b_templates:
        res_str = json.dumps(b_res)
        # Replace specific strings to avoid breaking backend-pool, etc.
        res_str = res_str.replace('_vm_b', f'_vm_{letter}').replace('-vm-b', f'-vm-{letter}')
        res_str = res_str.replace('_VM_B', f'_VM_{letter.upper()}').replace('-VM-B', f'-VM-{letter.upper()}')
        res_str = res_str.replace('_chat_server_b', f'_chat_server_{letter}').replace('-chat-server-b', f'-chat-server-{letter}')
        
        new_res = json.loads(res_str)
        
        # Dynamic IP for NIC
        if new_res['type'] == 'Microsoft.Network/networkInterfaces':
            for ipconfig in new_res['properties']['ipConfigurations']:
                ipconfig['properties']['privateIPAllocationMethod'] = 'Dynamic'
                if 'privateIPAddress' in ipconfig['properties']:
                    del ipconfig['properties']['privateIPAddress']
                    
        # Add customData for VM
        if new_res['type'] == 'Microsoft.Compute/virtualMachines':
            # Fix VM size since it got replaced by _B to _C, _D, etc.
            if 'hardwareProfile' in new_res['properties'] and 'vmSize' in new_res['properties']['hardwareProfile']:
                new_res['properties']['hardwareProfile']['vmSize'] = 'Standard_B2s_v2'
                
            if letter != 'b':
                if 'osProfile' not in new_res['properties']:
                    new_res['properties']['osProfile'] = {}
                new_res['properties']['osProfile']['customData'] = get_setup_script(letter)
            
        new_resources.append(new_res)

template['resources'] = new_resources

# Update Parameters
new_parameters = {}
for k, v in template['parameters'].items():
    if '_b' in k:
        pass # Skip b, we will regenerate it
    elif 'chat_server_a' in k or 'chat_server_b' in k:
        pass # Skip container instances
    else:
        new_parameters[k] = v

for letter in letters:
    new_parameters[f'virtualMachines_chat_vm_{letter}_name'] = {"type": "String", "defaultValue": f"chat-vm-{letter}"}
    new_parameters[f'networkInterfaces_chat_vm_{letter}VMNic_name'] = {"type": "String", "defaultValue": f"chat-vm-{letter}VMNic"}
    new_parameters[f'publicIPAddresses_chat_vm_{letter}PublicIP_name'] = {"type": "String", "defaultValue": f"chat-vm-{letter}PublicIP"}
    new_parameters[f'networkSecurityGroups_chat_vm_{letter}NSG_name'] = {"type": "String", "defaultValue": f"chat-vm-{letter}NSG"}

template['parameters'] = new_parameters

with open('chat_sfu_template_26.json', 'w', encoding='utf-8') as f:
    json.dump(template, f, indent=2)

# Update params.json
new_params_json = {"$schema": "https://schema.management.azure.com/schemas/2019-04-01/deploymentParameters.json#", "contentVersion": "1.0.0.0", "parameters": {}}
for k, v in params['parameters'].items():
    if '_b' in k:
        pass
    elif 'chat_server_a' in k or 'chat_server_b' in k:
        pass
    else:
        new_params_json['parameters'][k] = v

for letter in letters:
    new_params_json['parameters'][f'virtualMachines_chat_vm_{letter}_name'] = {"value": f"chat-vm-{letter}"}
    new_params_json['parameters'][f'networkInterfaces_chat_vm_{letter}VMNic_name'] = {"value": f"chat-vm-{letter}VMNic"}
    new_params_json['parameters'][f'publicIPAddresses_chat_vm_{letter}PublicIP_name'] = {"value": f"chat-vm-{letter}PublicIP"}
    new_params_json['parameters'][f'networkSecurityGroups_chat_vm_{letter}NSG_name'] = {"value": f"chat-vm-{letter}NSG"}

with open('params_26.json', 'w', encoding='utf-8') as f:
    json.dump(new_params_json, f, indent=2)

print("Generated chat_sfu_template_26.json and params_26.json successfully with 26 servers!")
