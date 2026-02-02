Import("env")
import json
import os
import hashlib

def json_creator(source, target, env):
    fw_version = 0                 
    for line in env.get("CPPDEFINES",[]):
        if isinstance(line, tuple):
            if "VERSION_NUMBER" == line[0].strip():
                fw_version = line[1]
                break
    data = {
        "platform": env.get("PIOENV"),
        "version": str(fw_version),
        "md5": str(hashlib.md5(open(source[0].get_abspath(),'rb').read()).hexdigest()),
    }
    json_output = json.dumps(data, indent=3)
    print("Created json: \n" + json_output)
    program_path = os.path.split(os.path.abspath(source[0].get_abspath()))[0]
    file_path = os.path.join(program_path, "info.json")
    with open(file_path, "w") as file:
        file.write(json_output)
    print("Json path: ", file_path)

env.AddPostAction("buildprog", json_creator)
