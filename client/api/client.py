import requests
from client.config import SERVER_URL

class ApiClient:
    @staticmethod
    def _request(method, endpoint, data=None):
        url = f"{SERVER_URL}{endpoint}"
        try:
            response = requests.request(
                method=method,
                url=url,
                json=data,
                timeout=10
            )
            if 200 <= response.status_code < 300:
                try:
                    return response.json(), None
                except ValueError:
                    return {"status": "ok"}, None
            else:
                try:
                    err_msg = response.json().get("error", f"Server returned {response.status_code}")
                except:
                    err_msg = f"Server returned {response.status_code}"
                return None, err_msg
        except requests.exceptions.RequestException as e:
            return None, str(e)

    # Server
    @staticmethod
    def get_health():
        return ApiClient._request("GET", "/api/system/health")

    @staticmethod
    def post_shutdown():
        # Using the requested endpoint from the prompt
        return ApiClient._request("POST", "/api/system/shutdown")
    
    @staticmethod
    def get_gpu_info():
        return ApiClient._request("GET", "/api/system/gpu")

    # VM Profiles
    @staticmethod
    def get_profiles():
        return ApiClient._request("GET", "/api/vm/profiles")

    @staticmethod
    def post_profile(data: dict):
        return ApiClient._request("POST", "/api/vm/profiles", data)

    @staticmethod
    def put_profile(profile_id: str, data: dict):
        return ApiClient._request("PUT", f"/api/vm/profiles/{profile_id}", data)

    @staticmethod
    def get_storage_disks():
        return ApiClient._request("GET", "/api/vm/storage/disks")

    # VM Lifecycle
    @staticmethod
    def get_vm_status(vm_id: str):
        return ApiClient._request("GET", f"/api/vm/{vm_id}/status")

    @staticmethod
    def post_vm_define(vm_id: str):
        return ApiClient._request("POST", f"/api/vm/{vm_id}/define")

    @staticmethod
    def post_vm_start(vm_id: str):
        return ApiClient._request("POST", f"/api/vm/{vm_id}/start")

    @staticmethod
    def post_vm_stop(vm_id: str):
        return ApiClient._request("POST", f"/api/vm/{vm_id}/stop")

    # Snapshots
    @staticmethod
    def get_snapshots(vm_id: str):
        return ApiClient._request("GET", f"/api/vm/{vm_id}/snapshots")

    @staticmethod
    def post_snapshot(vm_id: str, name: str = None):
        data = {"name": name} if name else {}
        return ApiClient._request("POST", f"/api/vm/{vm_id}/snapshot", data)

    @staticmethod
    def post_restore(vm_id: str, name: str = None):
        data = {"name": name} if name else {}
        return ApiClient._request("POST", f"/api/vm/{vm_id}/restore", data)

    # Fuzzing
    @staticmethod
    def post_fuzz_start(config: dict):
        return ApiClient._request("POST", "/api/fuzz/start", config)

    @staticmethod
    def post_fuzz_stop():
        return ApiClient._request("POST", "/api/fuzz/stop")

    @staticmethod
    def get_fuzz_status():
        return ApiClient._request("GET", "/api/fuzz/status")

    @staticmethod
    def get_fuzz_history():
        return ApiClient._request("GET", "/api/fuzz/history")
