import sys
import os

sys.stdout.reconfigure(encoding='utf-8')

_base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def _documents_dir():
    """Windows 문서 폴더의 실제 경로를 레지스트리에서 조회.

    문서 폴더가 OneDrive 로 리디렉션되어 있거나 한글 로캘("문서")인 경우
    ~/Documents 는 존재하지 않는다. 조회 실패시 None.
    """
    try:
        import winreg
    except ImportError:
        return None  # Windows 아님

    try:
        key = r"Software\Microsoft\Windows\CurrentVersion\Explorer\User Shell Folders"
        with winreg.OpenKey(winreg.HKEY_CURRENT_USER, key) as handle:
            raw, _ = winreg.QueryValueEx(handle, "Personal")
        return os.path.expandvars(raw)
    except OSError:
        return None


def _candidate_paths():
    """SecureOTA/scripts 후보 경로를 우선순위 순으로 생성 (중복 제거)."""
    home = os.path.expanduser("~")
    onedrive = os.environ.get("OneDrive") or os.environ.get("OneDriveConsumer")

    roots = []
    documents = _documents_dir()
    if documents:
        roots.append(os.path.join(documents, "Arduino"))
    if onedrive:
        roots.append(os.path.join(onedrive, "문서", "Arduino"))
        roots.append(os.path.join(onedrive, "Documents", "Arduino"))
    roots.append(os.path.join(home, "Documents", "Arduino"))
    roots.append(os.path.join(home, "Arduino"))

    paths = [os.path.join(r, "libraries", "SecureOTA", "scripts") for r in roots]
    paths.append(os.path.join(_base_dir, "..", "SecureOTA", "scripts"))  # 형제 폴더 (개발 환경)

    seen = set()
    result = []
    for path in paths:
        path = os.path.normpath(path)
        if path not in seen:
            seen.add(path)
            result.append(path)
    return result


def _find_secureota_scripts():
    """deploy_core.py 가 실제로 존재하는 경로를 반환. 없으면 None."""
    for path in _candidate_paths():
        if os.path.exists(os.path.join(path, "deploy_core.py")):
            return path
    return None


if __name__ == "__main__":
    _scripts_dir = _find_secureota_scripts()
    if _scripts_dir is None:
        print("SecureOTA 라이브러리를 찾을 수 없습니다.")
        print("Arduino IDE 에서 SecureOTA 라이브러리를 설치하세요.")
        print("\n탐색한 경로:")
        for _path in _candidate_paths():
            print(f"  - {_path}")
        sys.exit(1)

    sys.path.insert(0, _scripts_dir)

    from deploy_core import run_deploy
    run_deploy(_base_dir)
