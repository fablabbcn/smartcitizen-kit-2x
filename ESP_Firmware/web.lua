--------------------------------------
-- Starts Web server
--------------------------------------

if srv ~= nil then srv:close() end

srv = net.createServer(net.TCP)
srv:listen(80, function(conn)
     conn:on("receive", function(conn,payload)

          print(payload)

          local address = payload:match("(/[^%s]+)")
          
          if address == "/api/ap" then 
               sck.getAPlist( function(t)

                                   -- local i = 0
                                   -- local s = (i*1024)+1
                                   -- -- Send payload in chunks of 1024 bytes
                                   -- while s < string.len(t) do
                                   --      -- print("del "..s.." al "..(i+1)*1024)
                                   --      -- print( string.sub(t, s, (i+1)*1024) )
                                   --      conn:send(string.sub(t, s, (i+1)*1024))
                                   --      i = i + 1
                                   --      s = (i*1024)+1
                                   -- end
                                   
                                   conn:send(t) 
                              end)
          else 
               conn:send("<h1>Smartcitizen Kit Web server</h1>") 
          end
               
     end)
     conn:on("sent", function(conn) conn:close() end)
end)
