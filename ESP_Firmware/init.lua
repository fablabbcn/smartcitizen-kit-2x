sck = require("main")
if not file.exists("config.lua") then
		sck.saveConf()
	end
dofile("config.lua")
wifi.setmode(wifi.STATION)
wifi.sta.autoconnect(1)
sck.start()
-- turn off echo
uart.setup(0, 115200, 8, uart.PARITY_NONE, uart.STOPBITS_1, 0)



