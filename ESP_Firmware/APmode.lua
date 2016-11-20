--------------------------------------
-- Starts APmode
--------------------------------------
local hn = wifi.sta.gethostname()
if wifi.getmode() ~= wifi.STATIONAP then
    wifi.setmode(wifi.STATIONAP)
    wifi.ap.config({ssid=hn, pwd="smartcitizen"})
end
