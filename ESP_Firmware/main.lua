module = {}


local msg = {
          ESP_WIFI_CONNECTED       = 1,
          ESP_WIFI_ERROR           = 2,
          ESP_WIFI_ERROR_PASS      = 3,
          ESP_WIFI_ERROR_AP        = 4,
          ESP_TIME_FAIL            = 5,
          ESP_TIME_NEW             = 6,
          ESP_MODE_AP              = 7,
          ESP_MODE_STA             = 8,
          ESP_WEB_STARTED          = 9,
          ESP_MQTT_HELLO_OK        = 10,
          ESP_MQTT_PUBLISH_OK      = 11,
          ESP_MQTT_ERROR           = 12,
          ESP_HOSTNAME_UPDATED     = 13

     }

--------------------------------------
-- Configuration Management
--------------------------------------
--
function module.saveConf()
     if file.open("config.lua", "w") ~= nil then
          local s,p = wifi.sta.getconfig()
          file.writeline('wifi.sta.config("' .. s .. '", "' .. p .. '")')
          file.writeline('config = {}')

          if config == nil then config = {} end
          
          if config.hostName == nil then 
              local mac = wifi.ap.getmac()
               config.hostName = "SmartCitizen" .. string.sub(mac,-5,-4) .. string.sub(mac,-2,-1)
          end
          file.writeline('config.hostName = "' .. config.hostName .. '"')
          
          if config.token == nil then config.token = "null" end
          file.writeline('config.token = "' .. config.token .. '"')

          file.close()
          return true
     end
     return false
end

function module.factoryReset()
     config = nil
     module.saveConf()
end

function module.updateHostName()
     -- local mac = wifi.ap.getmac()
     -- config.hostName = "SmartCitizen" .. string.sub(mac,-5,-4) .. string.sub(mac,-2,-1)
     config.hostName = nil
     if module.saveConf() then print(msg.ESP_HOSTNAME_UPDATED) end
end

--------------------------------------
-- Leds
--------------------------------------
--Class for controlling individual Led
local function Led(myPin)
     local self = {
          state = 0
     }

     local Pin = myPin

     function self.set(newState)
          if newState == 2 then
               self.state = 1 - self.state
          else
               self.state = newState
               tmr.unregister(Pin)
          end
          gpio.write(Pin, 1 - self.state)
     end
     function self.blink(lapse)
          tmr.alarm(Pin, lapse, tmr.ALARM_AUTO, function() self.set(2) end)
     end
   return self
end

gpio.mode(1, gpio.OUTPUT)
gpio.mode(2, gpio.OUTPUT)
lLED = Led(1)       -- GPIO5
rLED = Led(2)       -- GPIO4


function module.ISOtime()
     local tm = rtctime.epoch2cal(rtctime.get())
     return string.format("%04d-%02d-%02dT%02d:%02d:%02dZ", tm["year"], tm["mon"], tm["day"], tm["hour"], tm["min"], tm["sec"])
end

--------------------------------------
--AP Info
--------------------------------------
-- Returns Acces Point list with timestamp
function module.getAPlist(callback)
     wifi.sta.getap(function (myTable)
          local apList = {}
          apList.AP = {}
          for key,value in pairs(myTable) do
               local ll = string.gmatch(value, '([^,]+)')              
               table.insert(apList.AP, {["SSID"]=key, ["Auth"]=ll(), ["RSSI"]=ll(), ["BSSID"]=ll(), ["Ch"]=ll()})
          end
          apList.time = module.ISOtime()
          callback(cjson.encode(apList))
     end)
end


--------------------------------------
-- MQTT start
--------------------------------------
function module.mqttConnect(action)
      module.mclient = mqtt.Client(config.token, 120, "user", "password", 0)
      module.mclient:connect("mqtt.smartcitizen.me", 1883, 0, action, function(client, reason) print(msg.ESP_MQTT_ERROR) end)
      -- Se deberia enviar la razon del fallo
end

--------------------------------------
-- MQTT Hello
--------------------------------------
function module.hello()
     local topic = "device/sck/" .. config.token .. "/hello"
     local mess = config.token .. ":Hello"

     module.mqttConnect(function(client)
          module.mclient:publish(topic, mess, 1, 0, function(client) 
               print(msg.ESP_MQTT_HELLO_OK)
               module.saveConf()
          end)
     end)
end


--------------------------------------
-- MQTT publish
--------------------------------------
function module.publish(payload)
     local topic = "device/sck/" .. config.token .. "/readings"
     local data = cjson.decode(payload)

     local mess = ""

     mess = '{"data":[{"recorded_at":"' .. data.time .. '","sensors":[{"id":29,"value":' .. data.noise ..'},{"id":13,"value":' .. data.humidity ..'},{"id":12,"value":' .. data.temperature ..'},{"id":10,"value":' .. data.battery .. '}]}]}'
     
     module.mqttConnect(function(client) module.mclient:publish(topic, mess, 1, 0, function(client) print(msg.ESP_MQTT_PUBLISH_OK) end) end)

end

function module.getTime()
     sntp.sync("pool.ntp.org", function(sec) print(msg.ESP_TIME_NEW, sec) end, function() print(msg.ESP_TIME_FAIL) end)
end

function module.netStatus()
     local netStatus = wifi.sta.status()
     if netStatus == wifi.STA_IDLE then wifi.sta.connect()
     elseif netStatus == wifi.STA_WRONGPWD then print(msg.ESP_WIFI_ERROR_PASS)
     elseif netStatus == wifi.STA_APNOTFOUND then print(msg.ESP_WIFI_ERROR_AP)
     elseif netStatus == wifi.STA_FAIL then print(msg.ESP_WIFI_ERROR)
     elseif netStatus == wifi.STA_GOTIP then print(msg.ESP_WIFI_CONNECTED) end
end

--------------------------------------
-- Start SCK
--------------------------------------
-- Starts network and check everything is OK
function module.start()

     print("")

     lLED.blink(350)
     rLED.blink(350)

     -- Hostname
     wifi.sta.sethostname(config.hostName)

     wifi.sta.eventMonReg(wifi.STA_WRONGPWD, function() print(msg.ESP_WIFI_ERROR_PASS) end)
     wifi.sta.eventMonReg(wifi.STA_APNOTFOUND, function() print(msg.ESP_WIFI_ERROR_AP) end)
     wifi.sta.eventMonReg(wifi.STA_FAIL, function()
          print(msg.ESP_WIFI_ERROR)
          lLED.blink(350)
          rLED.blink(350)
          require("APmode")
          print(msg.ESP_MODE_AP)
          require("web")
          print(msg.ESP_WEB_STARTED)
     end)
     wifi.sta.eventMonReg(wifi.STA_GOTIP,    function()
                                                  print(msg.ESP_WIFI_CONNECTED)
                                                  tmr.stop(6)
                                                  lLED.set(1)
                                                  rLED.set(0)
                                                  module.saveConf()

                                                  if wifi.getmode() ~= wifi.STATION then 
                                                       print(msg.ESP_MODE_STA)
                                                       wifi.setmode(wifi.STATION) 
                                                  end

     end)
     wifi.sta.eventMonStart()
end


return module