$client = New-Object System.Net.Sockets.TcpClient('192.168.127.196', 23)
$stream = $client.GetStream()
$reader = New-Object System.IO.StreamReader($stream)
while ($client.Connected) { $reader.ReadLine() }