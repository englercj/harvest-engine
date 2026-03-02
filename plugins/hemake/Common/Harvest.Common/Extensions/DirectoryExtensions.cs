namespace Harvest.Common.Extensions;

public static class DirectoryExtensions
{
    public static void CreateParentDirectory(string path)
    {
        if (Path.GetDirectoryName(path) is string parent)
        {
            Directory.CreateDirectory(parent);
        }
    }
}
