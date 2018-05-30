using System;
using System.IO.MemoryMappedFiles;

class Client {
    public static void Main() {

        var shm = MemoryMappedFile.OpenExisting("shared_memory", MemoryMappedFileRights.ReadWrite);
        var view = shm.CreateViewAccessor();

        var data = new byte[view.Capacity];
        view.ReadArray<byte>(0, data, 0, data.Length);

        var str = System.Text.Encoding.UTF8.GetString(data, 0, data.Length);

        Console.WriteLine(str);

        view.Write(0, 0);
    }
}