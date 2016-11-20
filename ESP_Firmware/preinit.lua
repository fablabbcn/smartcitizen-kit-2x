-- turn off echo
uart.setup(0, 115200, 8, uart.PARITY_NONE, uart.STOPBITS_1, 0)

if file.exists("config.lua") then
	dofile("config.lua")
	wifi.setmode(wifi.STATION)
	wifi.sta.autoconnect(1)
	sck = require("main")
	sck.start()
else
	sck = require("main")
	sck.saveConf()
	node.restart()
end



