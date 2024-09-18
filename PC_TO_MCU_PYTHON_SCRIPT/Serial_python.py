import serial.tools.list_ports

#list of all serial ports
ports = serial.tools.list_ports.comports()
serialInst = serial.Serial()

#printing all serial ports available
portList =[]
for oneport in ports:
    portList.append(str(oneport))
    print(str(oneport))

#enabling user to select port
val = input("select port: COM")

print(val)

#stating the port selected
for x in range(0,len(portList)):
    if portList[x].startswith("COM"+str(val)):
        portVar =  "COM" +str(val)
        print(portList[x])

#declaring all the wanted uart definitions
serialInst.baudrate = 2400
serialInst.port = portVar
serialInst.open()




data = """
Finance Minister Arun Jaitley Tuesday hit out at former RBI governor Raghuram Rajan for predicting that the next banking crisis would be triggered by MSME lending, saying postmortem is easier than taking action when it was required.Rajan, who had as the chief economist at IMF warned of impending financial crisis of 2008, in a note to a parliamentary committee warned against ambitious credit targets and loan waivers, saying that they could be the sources of next banking crisis. Government should focus on sources of the next crisis, not just the last one.In particular, government should refrain from setting ambitious credit targets or waiving loans.Credit targets are sometimes achieved by abandoning appropriate due diligence, creating the environment for future NPAs," Rajan said in the note."Both MUDRA loans as well as the Kisan Credit Card, while popular, have to be examined more closely for potential credit risk. Rajan, who was RBI governor for three years till September 2016, is currently.
"""
paragraph_bytes = data.encode('utf-8')

print("\n Sending data to the port")

# Send the entire paragraph to the serial port
try:
    serialInst.write(paragraph_bytes)
    serialInst.flush()  # Ensure the data is transmitted
    print("Data sent to the serial port successfully!")
except Exception as e:
    print(f"Error sending data: {e}")


print("\nReading from the MCU:")
while True:
    if serialInst.in_waiting:
        packet = serialInst.readline()  # Indented under the if statement
        print(packet.decode('utf-8').rstrip('\n'))   # Indented as part of the if block
