import win32com.client as cl
import requests
import zipfile
import shutil
import os
import io

shell = cl.Dispatch("WScript.Shell")
shortcut_pos = os.path.join(shell.SpecialFolders("Desktop"),"Platformer.lnk")
print("WELCOME TO THE SETUP WIZARD. The commands are exit, install, update, uninstall")
target = os.path.join(os.environ["LOCALAPPDATA"], "Platformer")
command = -1
while command != "exit":
    command = input(">>> ")
    if command == "install":
        if os.path.exists(target) and input("Are you sure? This action will delete "+target).lower() in ["yes", "y"]:
            shutil.rmtree(target)
        path = "https://github.com/catlovergg1234/sdl-platformer/archive/refs/heads/refs.zip"
        with requests.get(path, stream=True) as resp:
            resp.raise_for_status()

            total = int(resp.headers.get("Content-Length", 0))
            downloaded = 0
            data = bytearray()

            for chunk in resp.iter_content(chunk_size=8192):
                if chunk:
                    data.extend(chunk)
                    downloaded += len(chunk)

                    percent = downloaded / total * 100 if total else 0
                    dots = "." * ((downloaded // 8192) % 4)

                    print(
                        f"\rDownloading ({percent:.0f}%){dots:<3}",
                        end="",
                        flush=True
                    )

        print("\nDownload complete!")
        with zipfile.ZipFile(io.BytesIO(data)) as file:
            file.extractall(target)
            print("Placed downloaded files at ", os.environ["LOCALAPPDATA"], "Platformer.", sep="")
        shortcut_data = input("Create desktop shortcut?: ")
        while shortcut_data.lower() not in ["y", "n", "yes", "no"]:
            print("Invalid, please type y, n, yes or no.")
            shortcut_data = input("Create desktop shortcut?: ")

        if shortcut_data.lower() in ["yes","y"]:
            shortcut = shell.CreateShortCut(shortcut_pos)
            shortcut.Targetpath = os.path.join(target, "main.exe")
            shortcut.WorkingDirectory = target
            shortcut.save()

        print("You have downloaded Platformer :)")
    if command == "uninstall":
        if not os.path.exists(target):
            print("Error: You haven't installed the game yet. Use install for updating as well.")
            continue
        if input("Are you sure? This action will delete "+target).lower() in ["yes", "y"]:
            shutil.rmtree(target)
        else:
            continue
        if os.path.exists(shortcut_pos):
            os.remove(shortcut_pos)
