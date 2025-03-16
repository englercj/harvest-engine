// Copyright Chad Engler

using System.Xml;

namespace Harvest.Make.Extensions;

public static partial class XmlWriterExtensions
{
    public static void WriteElementBool(this XmlWriter writer, string localName, bool value)
    {
        writer.WriteElementString(localName, value ? "true" : "false");
    }
}
