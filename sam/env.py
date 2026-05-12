from os.path import isfile
Import("env")

file_name = env.GetProjectOption("file_name")
print (f"Checking {file_name} environment")

if not isfile(file_name):
  print ("File doesnt exist")
  l={
    "SC_TEST_WIFI_SSID":"fake",
    "SC_TEST_WIFI_PASSWD":"fake"
  }
  env.Append(CPPDEFINES=[
    (l[0], env.StringifyMacro(l[1])),
  ])
else:
  try:
    f = open(file_name, "r")
    lines = f.readlines()
    for line in lines:
      l = line.strip().split("=")
      print (f'Adding {l[0]}: {l[1]}')
      env.Append(CPPDEFINES=[
          (l[0], env.StringifyMacro(l[1])),
      ])
  except IOError:
    print(f"File {file_name} not accessible",)
  finally:
    f.close()