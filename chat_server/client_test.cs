using System;
using System.Net.Sockets;
using System.Text;
using System.Threading;

class Program {
    static void Main() {
        var client = new TcpClient("chat-sfu-lb-2026.japaneast.cloudapp.azure.com", 4000);
        var stream = client.GetStream();
        var reader = new byte[4096];
        int bytes = stream.Read(reader, 0, reader.Length);
        Console.WriteLine("RECV: " + Encoding.UTF8.GetString(reader, 0, bytes));

        string joinMsg = "{\"type\":\"join\",\"secretId\":\"test-secret\",\"userName\":\"tester\",\"roomName\":\"Public\",\"pub\":true,\"nclr\":0}\n";
        var data = Encoding.UTF8.GetBytes(joinMsg);
        stream.Write(data, 0, data.Length);

        Thread.Sleep(1000);
        bytes = stream.Read(reader, 0, reader.Length);
        Console.WriteLine("RECV: " + Encoding.UTF8.GetString(reader, 0, bytes));
    }
}
