import os
import sys
import json
import time
import zipfile
import xml.etree.ElementTree as ET
import requests

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")


def load_env_file(env_path):
    if not os.path.exists(env_path):
        return
    with open(env_path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#") or "=" not in line:
                continue
            k, v = line.split("=", 1)
            os.environ.setdefault(k.strip(), v.strip())


load_env_file(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".env.store"))

TENANT_ID = os.environ.get("STORE_TENANT_ID", "")
CLIENT_ID = os.environ.get("STORE_CLIENT_ID", "")
CLIENT_SECRET = os.environ.get("STORE_CLIENT_SECRET", "")
PRODUCT_ID = os.environ.get("STORE_PRODUCT_ID", "9MVJZSR56JJK")


def get_manifest_version(root_dir):
    manifest_path = os.path.join(root_dir, "package", "Package.appxmanifest")
    tree = ET.parse(manifest_path)
    root = tree.getroot()
    for elem in root.iter():
        if elem.tag.endswith("Identity") and "Version" in elem.attrib:
            return elem.attrib["Version"]
    raise RuntimeError("Could not find Identity Version in Package.appxmanifest")


def get_oauth_token():
    if not TENANT_ID or not CLIENT_ID or not CLIENT_SECRET:
        raise RuntimeError(
            "Missing STORE_TENANT_ID, STORE_CLIENT_ID, or STORE_CLIENT_SECRET. "
            "Set them in environment variables or tools/.env.store."
        )
    print("Acquiring Microsoft Store API OAuth token...")
    token_url = f"https://login.microsoftonline.com/{TENANT_ID}/oauth2/token"
    data = {
        "grant_type": "client_credentials",
        "client_id": CLIENT_ID,
        "client_secret": CLIENT_SECRET,
        "resource": "https://manage.devcenter.microsoft.com",
    }
    resp = requests.post(token_url, data=data)
    resp.raise_for_status()
    return resp.json()["access_token"]


def get_or_create_submission(headers):
    app_url = f"https://manage.devcenter.microsoft.com/v1.0/my/applications/{PRODUCT_ID}"
    app_resp = requests.get(app_url, headers=headers)
    app_resp.raise_for_status()
    app_data = app_resp.json()

    pending = app_data.get("pendingApplicationSubmission")
    if pending and pending.get("id"):
        sub_id = pending["id"]
        print(f"Found existing draft submission: {sub_id}")
        sub_url = f"{app_url}/submissions/{sub_id}"
        sub_resp = requests.get(sub_url, headers=headers)
        sub_resp.raise_for_status()
        return sub_resp.json()

    print("Creating new draft submission...")
    create_url = f"{app_url}/submissions"
    create_resp = requests.post(create_url, headers=headers)
    create_resp.raise_for_status()
    new_sub = create_resp.json()
    print(f"Created new draft submission: {new_sub.get('id')}")
    return new_sub


def upload_and_update_submission(headers, submission_data, bundle_path):
    sub_id = submission_data["id"]
    file_upload_url = submission_data["fileUploadUrl"]
    bundle_filename = os.path.basename(bundle_path)

    zip_path = os.path.splitext(bundle_path)[0] + ".zip"
    print(f"Packaging {bundle_filename} into zip: {zip_path}...")
    with zipfile.ZipFile(zip_path, "w", compression=zipfile.ZIP_DEFLATED) as zf:
        zf.write(bundle_path, arcname=bundle_filename)

    print(f"Uploading zip ({os.path.getsize(zip_path)} bytes) to Store Blob Storage...")
    upload_url = file_upload_url.replace("+", "%2B")
    with open(zip_path, "rb") as f:
        up_resp = requests.put(
            upload_url,
            data=f,
            headers={"x-ms-blob-type": "BlockBlob"},
        )
    up_resp.raise_for_status()
    print("Zip package uploaded successfully.")

    existing_packages = submission_data.get("applicationPackages", [])
    pkg_filenames = [p.get("fileName") for p in existing_packages]

    if bundle_filename not in pkg_filenames:
        new_packages = []
        for pkg in existing_packages:
            if pkg.get("id"):
                pkg_copy = dict(pkg)
                pkg_copy["fileStatus"] = "PendingDelete"
                new_packages.append(pkg_copy)
        new_packages.append(
            {
                "fileName": bundle_filename,
                "fileStatus": "PendingUpload",
                "minimumDirectXVersion": "None",
                "minimumSystemRam": "None",
            }
        )
        submission_data["applicationPackages"] = new_packages

    print(f"Updating submission {sub_id}...")
    put_url = f"https://manage.devcenter.microsoft.com/v1.0/my/applications/{PRODUCT_ID}/submissions/{sub_id}"
    put_resp = requests.put(put_url, headers=headers, json=submission_data)
    if put_resp.status_code != 200:
        raise RuntimeError(f"Failed to update submission: {put_resp.status_code} - {put_resp.text}")
    print("Submission updated successfully.")


def commit_submission(headers, submission_id):
    print(f"Committing submission {submission_id} to Microsoft Store...")
    commit_url = f"https://manage.devcenter.microsoft.com/v1.0/my/applications/{PRODUCT_ID}/submissions/{submission_id}/commit"
    res = requests.post(commit_url, headers=headers)
    if res.status_code not in (200, 202):
        raise RuntimeError(f"Failed to commit submission: {res.status_code} - {res.text}")
    print("Submission committed! It is now queued for certification.")


def poll_submission_status(headers, submission_id, max_attempts=12, interval=5):
    status_url = f"https://manage.devcenter.microsoft.com/v1.0/my/applications/{PRODUCT_ID}/submissions/{submission_id}/status"
    for i in range(max_attempts):
        time.sleep(interval)
        res = requests.get(status_url, headers=headers)
        if res.status_code == 200:
            status_data = res.json()
            st = status_data.get("status")
            print(f"Status check ({i + 1}/{max_attempts}): {st}")
            if status_data.get("statusDetails", {}).get("errors"):
                print("Errors:", json.dumps(status_data["statusDetails"]["errors"], indent=2, ensure_ascii=False))
            if st not in ("CommitStarted", "PendingCommit"):
                return status_data
    return {"status": "CommitProcessing"}


def main():
    root_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    full_version = get_manifest_version(root_dir)  # e.g., 1.0.21.0
    bundle_path = os.path.join(
        root_dir,
        "package",
        "AppPackages",
        f"package_{full_version}_Test",
        f"package_{full_version}_x64.appxbundle",
    )
    if not os.path.exists(bundle_path):
        raise FileNotFoundError(f"Bundle not found: {bundle_path}")

    print(f"=== Publishing chat.exe v{full_version} to Microsoft Store ===")
    token = get_oauth_token()
    headers = {
        "Authorization": f"Bearer {token}",
        "Content-Type": "application/json",
    }

    sub_data = get_or_create_submission(headers)
    sub_id = sub_data["id"]

    upload_and_update_submission(headers, sub_data, bundle_path)
    commit_submission(headers, sub_id)
    final_status = poll_submission_status(headers, sub_id)
    print(f"Final submission status: {json.dumps(final_status, ensure_ascii=False)}")


if __name__ == "__main__":
    main()
