// Copyright Chad Engler

using System.Xml;

namespace Harvest.Make.Extensions;

public static partial class XmlWriterExtensions
{
    public static void TryWriteElementString(this XmlWriter writer, string localName, string? value)
    {
        if (value is not null)
        {
            writer.WriteElementString(localName, value);
        }
    }

    public static void TryWriteElementBool(this XmlWriter writer, string localName, bool? value)
    {
        if (value is not null)
        {
            writer.WriteElementString(localName, value.Value ? "true" : "false");
        }
    }

    public static void WriteElementBool(this XmlWriter writer, string localName, bool value)
    {
        writer.WriteElementString(localName, value ? "true" : "false");
    }

    public static void WriteElementBoolIfTrue(this XmlWriter writer, string localName, bool value)
    {
        if (value)
        {
            writer.WriteElementString(localName, "true");
        }
    }
}
