// Copyright Chad Engler

#include "he/core/kdl_document.h"

#include "fixtures.h"

#include "he/core/directory.h"
#include "he/core/file.h"
#include "he/core/hash_table.h"
#include "he/core/log.h"
#include "he/core/math.h"
#include "he/core/path.h"
#include "he/core/result.h"
#include "he/core/result_fmt.h"
#include "he/core/stopwatch.h"
#include "he/core/string_view.h"
#include "he/core/test.h"
#include "he/core/types.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, kdl_document, Construct)
{
    KdlDocument doc;
    HE_EXPECT_EQ_PTR(&doc.GetAllocator(), &Allocator::GetDefault());
    HE_EXPECT(doc.Nodes().IsEmpty());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, kdl_document, ConstructWithAllocator)
{
    CrtAllocator allocator;
    KdlDocument doc(allocator);
    HE_EXPECT_EQ_PTR(&doc.GetAllocator(), &allocator);
    HE_EXPECT(doc.Nodes().IsEmpty());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, kdl_document, Read)
{
    const StringView kdlDoc = GetTestKdlDocument();
    KdlDocument doc;
    const KdlReadResult r = doc.Read(kdlDoc);
    HE_EXPECT(r.IsValid(), r.error, r.line, r.column, r.expected);

    const Vector<KdlNode>& nodes = doc.Nodes();
    HE_EXPECT_EQ(nodes.Size(), 7);
    {
        const KdlNode& node = nodes[0];
        HE_EXPECT_EQ(node.Name(), "boolean");
        HE_EXPECT_EQ(node.Properties().Size(), 2);
        HE_EXPECT_EQ(node.Properties().Get("bool1").Bool(), true);
        HE_EXPECT_EQ(node.Properties().Get("bool2").Bool(), false);
    }
    // Integers of various formats
    {
        const KdlNode& node = nodes[1];
        HE_EXPECT_EQ(node.Name(), "integers");
        HE_EXPECT_EQ(node.Children().Size(), 5);
        {
            const KdlNode& child = node.Children()[0];
            HE_EXPECT_EQ(child.Name(), "decimal");
            HE_EXPECT_EQ(child.Properties().Size(), 4);
            HE_EXPECT_EQ(child.Properties().Get("int1").Uint(), 99);
            HE_EXPECT_EQ(child.Properties().Get("int2").Uint(), 42);
            HE_EXPECT_EQ(child.Properties().Get("int3").Uint(), 0);
            HE_EXPECT_EQ(child.Properties().Get("int4").Int(), -17);
        }
        {
            const KdlNode& child = node.Children()[1];
            HE_EXPECT_EQ(child.Name(), "decimal");
            HE_EXPECT_EQ(child.Properties().Size(), 4);
            HE_EXPECT_EQ(child.Properties().Get("int5").Uint(), 1000);
            HE_EXPECT_EQ(child.Properties().Get("int6").Uint(), 5349221);
            HE_EXPECT_EQ(child.Properties().Get("int7").Uint(), 5349221);
            HE_EXPECT_EQ(child.Properties().Get("int8").Uint(), 12345);
        }
        {
            const KdlNode& child = node.Children()[2];
            HE_EXPECT_EQ(child.Name(), "hexadecimal");
            HE_EXPECT_EQ(child.Properties().Size(), 3);
            HE_EXPECT_EQ(child.Properties().Get("hex1").Uint(), 0xdeadbeef);
            HE_EXPECT_EQ(child.Properties().Get("hex2").Uint(), 0xdeadbeef);
            HE_EXPECT_EQ(child.Properties().Get("hex3").Uint(), 0xdeadbeef);
        }
        {
            const KdlNode& child = node.Children()[3];
            HE_EXPECT_EQ(child.Name(), "octal");
            HE_EXPECT_EQ(child.Properties().Size(), 2);
            HE_EXPECT_EQ(child.Properties().Get("oct1").Uint(), 01234567);
            HE_EXPECT_EQ(child.Properties().Get("oct2").Uint(), 0755);
        }
        {
            const KdlNode& child = node.Children()[4];
            HE_EXPECT_EQ(child.Name(), "binary");
            HE_EXPECT_EQ(child.Properties().Size(), 1);
            HE_EXPECT_EQ(child.Properties().Get("bin1").Uint(), 0b11010110);
        }
    }
    // Doubles of various formats
    {
        const KdlNode& node = nodes[2];
        HE_EXPECT_EQ(node.Name(), "floats");
        HE_EXPECT_EQ(node.Children().Size(), 4);
        {
            const KdlNode& child = node.Children()[0];
            HE_EXPECT_EQ(child.Name(), "decimal");
            HE_EXPECT_EQ(child.Properties().Size(), 3);
            HE_EXPECT_EQ(child.Properties().Get("flt1").Float(), 1.0);
            HE_EXPECT_EQ(child.Properties().Get("flt2").Float(), 3.1415);
            HE_EXPECT_EQ(child.Properties().Get("flt3").Float(), -0.01);
        }
        {
            const KdlNode& child = node.Children()[1];
            HE_EXPECT_EQ(child.Name(), "exponent");
            HE_EXPECT_EQ(child.Properties().Size(), 3);
            HE_EXPECT_EQ(child.Properties().Get("flt4").Float(), 5e22);
            HE_EXPECT_EQ(child.Properties().Get("flt5").Float(), 1e06);
            HE_EXPECT_EQ(child.Properties().Get("flt6").Float(), -2e-2);
        }
        {
            const KdlNode& child = node.Children()[2];
            HE_EXPECT_EQ(child.Name(), "both");
            HE_EXPECT_EQ(child.Properties().Size(), 2);
            HE_EXPECT_EQ(child.Properties().Get("flt7").Float(), 6.626e-34);
            HE_EXPECT_EQ(child.Properties().Get("flt8").Float(), 224617.445991228);
        }
        {
            const KdlNode& child = node.Children()[3];
            HE_EXPECT_EQ(child.Name(), "special");
            HE_EXPECT_EQ(child.Properties().Size(), 3);
            HE_EXPECT_EQ(child.Properties().Get("flt9").Float(), Limits<double>::Infinity);
            HE_EXPECT_EQ(child.Properties().Get("flt10").Float(), -Limits<double>::Infinity);
            HE_EXPECT(IsNan(child.Properties().Get("flt11").Float()));
        }
    }
    // Strings of various formats
    {
        const KdlNode& node = nodes[3];
        HE_EXPECT_EQ(node.Name(), "strings");
        HE_EXPECT_EQ(node.Children().Size(), 15);
        {
            const KdlNode& child = node.Children()[0];
            HE_EXPECT_EQ(child.Name(), "escaped");
            HE_EXPECT_EQ(child.Arguments().Size(), 1);
            HE_EXPECT_EQ(child.Arguments()[0].String(), "I'm a string. \"You can quote me\". Name\tJosé\nLocation\tSF.");
        }
        {
            const KdlNode& child = node.Children()[1];
            HE_EXPECT_EQ(child.Name(), "multiline");
            HE_EXPECT_EQ(child.Arguments().Size(), 1);
            HE_EXPECT_EQ(child.Arguments()[0].String(), "Roses are red\nViolets are blue");
        }
        {
            const KdlNode& child = node.Children()[2];
            HE_EXPECT_EQ(child.Name(), "multiline2");
            HE_EXPECT_EQ(child.Arguments().Size(), 1);
            HE_EXPECT_EQ(child.Arguments()[0].String(), "Roses are red\nViolets are blue");
        }
        {
            const KdlNode& child = node.Children()[3];
            HE_EXPECT_EQ(child.Name(), "fox");
            HE_EXPECT_EQ(child.Arguments().Size(), 1);
            HE_EXPECT_EQ(child.Arguments()[0].String(), "The quick brown fox jumps over the lazy dog.");
        }
        {
            const KdlNode& child = node.Children()[4];
            HE_EXPECT_EQ(child.Name(), "fox2");
            HE_EXPECT_EQ(child.Arguments().Size(), 1);
            HE_EXPECT_EQ(child.Arguments()[0].String(), "The quick brown fox jumps over the lazy dog.");
        }
        {
            const KdlNode& child = node.Children()[5];
            HE_EXPECT_EQ(child.Name(), "fox3");
            HE_EXPECT_EQ(child.Arguments().Size(), 1);
            HE_EXPECT_EQ(child.Arguments()[0].String(), "The quick brown fox jumps over the lazy dog.");
        }
        {
            const KdlNode& child = node.Children()[6];
            HE_EXPECT_EQ(child.Name(), "quote1");
            HE_EXPECT_EQ(child.Arguments().Size(), 1);
            HE_EXPECT_EQ(child.Arguments()[0].String(), "Here are two quotation marks: \"\". Simple enough.");
        }
        {
            const KdlNode& child = node.Children()[7];
            HE_EXPECT_EQ(child.Name(), "quote2");
            HE_EXPECT_EQ(child.Arguments().Size(), 1);
            HE_EXPECT_EQ(child.Arguments()[0].String(), "Here are three quotation marks: \"\"\".");
        }
        {
            const KdlNode& child = node.Children()[8];
            HE_EXPECT_EQ(child.Name(), "quote3");
            HE_EXPECT_EQ(child.Arguments().Size(), 1);
            HE_EXPECT_EQ(child.Arguments()[0].String(), "Here are fifteen quotation marks: \"\"\"\"\"\"\"\"\"\"\"\"\"\"\".");
        }
        {
            const KdlNode& child = node.Children()[9];
            HE_EXPECT_EQ(child.Name(), "winpath");
            HE_EXPECT_EQ(child.Arguments().Size(), 1);
            HE_EXPECT_EQ(child.Arguments()[0].String(), "C:\\Users\\nodejs\\templates");
        }
        {
            const KdlNode& child = node.Children()[10];
            HE_EXPECT_EQ(child.Name(), "winpath2");
            HE_EXPECT_EQ(child.Arguments().Size(), 1);
            HE_EXPECT_EQ(child.Arguments()[0].String(), "\\\\ServerX\\admin$\\system32\\");
        }
        {
            const KdlNode& child = node.Children()[11];
            HE_EXPECT_EQ(child.Name(), "quoted");
            HE_EXPECT_EQ(child.Arguments().Size(), 1);
            HE_EXPECT_EQ(child.Arguments()[0].String(), "Tom \"Dubs\" Preston-Werner");
        }
        {
            const KdlNode& child = node.Children()[12];
            HE_EXPECT_EQ(child.Name(), "regex");
            HE_EXPECT_EQ(child.Arguments().Size(), 1);
            HE_EXPECT_EQ(child.Arguments()[0].String(), "<\\i\\c*\\s*>");
        }
        {
            const KdlNode& child = node.Children()[13];
            HE_EXPECT_EQ(child.Name(), "regex2");
            HE_EXPECT_EQ(child.Arguments().Size(), 1);
            HE_EXPECT_EQ(child.Arguments()[0].String(), "I [dw]on't need \\d{2} apples");
        }
        {
            const KdlNode& child = node.Children()[14];
            HE_EXPECT_EQ(child.Name(), "lines");
            HE_EXPECT_EQ(child.Arguments().Size(), 1);
            HE_EXPECT_EQ(child.Arguments()[0].String(), "    The first newline is\n    trimmed in multiline strings.\n    All other whitespace\n    is preserved.");
        }
    }
    // Nested nodes
    {
        const KdlNode& node = nodes[4];
        HE_EXPECT_EQ(node.Name(), "tables");
        HE_EXPECT_EQ(node.Children().Size(), 3);
        {
            const KdlNode& child = node.Children()[0];
            HE_EXPECT_EQ(child.Name(), "dotted.names.change.nothing");
            HE_EXPECT_EQ(child.Children().Size(), 1);
            {
                const KdlNode& contrib = child.Children()[0];
                HE_EXPECT_EQ(contrib.Name(), "contributors");
                HE_EXPECT_EQ(contrib.Children().Size(), 2);
                {
                    const KdlNode& user = contrib.Children()[0];
                    HE_EXPECT_EQ(user.Name(), "Foo Bar");
                    HE_EXPECT_EQ(user.Arguments().Size(), 1);
                    HE_EXPECT_EQ(user.Arguments()[0].String(), "foo@example.com");
                }
                {
                    const KdlNode& user = contrib.Children()[1];
                    HE_EXPECT_EQ(user.Name(), "Baz Qux");
                    HE_EXPECT_EQ(user.Properties().Size(), 2);
                    HE_EXPECT_EQ(user.Properties().Get("email").String(), "bazqux@example.com");
                    HE_EXPECT_EQ(user.Properties().Get("url").String(), "https://example.com/bazqux");
                }
            }
        }
        {
            const KdlNode& child = node.Children()[1];
            HE_EXPECT_EQ(child.Name(), "points");
            HE_EXPECT_EQ(child.Children().Size(), 3);
            {
                const KdlNode& point = child.Children()[0];
                HE_EXPECT_EQ(point.Name(), "point");
                HE_EXPECT_EQ(point.Properties().Size(), 3);
                HE_EXPECT_EQ(point.Properties().Get("x").Uint(), 1);
                HE_EXPECT_EQ(point.Properties().Get("y").Uint(), 2);
                HE_EXPECT_EQ(point.Properties().Get("z").Uint(), 3);
            }
            {
                const KdlNode& point = child.Children()[1];
                HE_EXPECT_EQ(point.Name(), "point");
                HE_EXPECT_EQ(point.Properties().Size(), 3);
                HE_EXPECT_EQ(point.Properties().Get("x").Uint(), 7);
                HE_EXPECT_EQ(point.Properties().Get("y").Uint(), 8);
                HE_EXPECT_EQ(point.Properties().Get("z").Uint(), 9);
            }
            {
                const KdlNode& point = child.Children()[2];
                HE_EXPECT_EQ(point.Name(), "point");
                HE_EXPECT_EQ(point.Properties().Size(), 3);
                HE_EXPECT_EQ(point.Properties().Get("x").Uint(), 2);
                HE_EXPECT_EQ(point.Properties().Get("y").Uint(), 4);
                HE_EXPECT_EQ(point.Properties().Get("z").Uint(), 8);
            }
        }
        {
            const KdlNode& child = node.Children()[2];
            HE_EXPECT_EQ(child.Name(), "points");
            HE_EXPECT_EQ(child.Children().Size(), 3);
            {
                const KdlNode& point = child.Children()[0];
                HE_EXPECT_EQ(point.Name(), "-");
                HE_EXPECT_EQ(point.Arguments().Size(), 3);
                HE_EXPECT_EQ(point.Arguments()[0].Uint(), 1);
                HE_EXPECT_EQ(point.Arguments()[1].Uint(), 2);
                HE_EXPECT_EQ(point.Arguments()[2].Uint(), 3);
            }
            {
                const KdlNode& point = child.Children()[1];
                HE_EXPECT_EQ(point.Name(), "-");
                HE_EXPECT_EQ(point.Arguments().Size(), 3);
                HE_EXPECT_EQ(point.Arguments()[0].Uint(), 7);
                HE_EXPECT_EQ(point.Arguments()[1].Uint(), 8);
                HE_EXPECT_EQ(point.Arguments()[2].Uint(), 9);
            }
            {
                const KdlNode& point = child.Children()[2];
                HE_EXPECT_EQ(point.Name(), "-");
                HE_EXPECT_EQ(point.Arguments().Size(), 3);
                HE_EXPECT_EQ(point.Arguments()[0].Uint(), 2);
                HE_EXPECT_EQ(point.Arguments()[1].Uint(), 4);
                HE_EXPECT_EQ(point.Arguments()[2].Uint(), 8);
            }
        }
    }
    // Website
    {
        const KdlNode& node = nodes[5];
        HE_EXPECT_EQ(node.Name(), "!doctype");
        HE_EXPECT_EQ(node.Arguments().Size(), 1);
        HE_EXPECT_EQ(node.Arguments()[0].String(), "html");
    }
    {
        const KdlNode& node = nodes[6];
        HE_EXPECT_EQ(node.Name(), "html");
        HE_EXPECT_EQ(node.Properties().Size(), 1);
        HE_EXPECT_EQ(node.Properties().Get("lang").String(), "en");
        HE_EXPECT_EQ(node.Children().Size(), 2);
        {
            const KdlNode& head = node.Children()[0];
            HE_EXPECT_EQ(head.Name(), "head");
            HE_EXPECT_EQ(head.Children().Size(), 5);
            {
                const KdlNode& title = head.Children()[0];
                HE_EXPECT_EQ(title.Name(), "meta");
                HE_EXPECT_EQ(title.Properties().Size(), 1);
                HE_EXPECT_EQ(title.Properties().Get("charset").String(), "utf-8");
            }
            {
                const KdlNode& meta = head.Children()[1];
                HE_EXPECT_EQ(meta.Name(), "meta");
                HE_EXPECT_EQ(meta.Properties().Size(), 2);
                HE_EXPECT_EQ(meta.Properties().Get("name").String(), "viewport");
                HE_EXPECT_EQ(meta.Properties().Get("content").String(), "width=device-width, initial-scale=1.0");
            }
            {
                const KdlNode& style = head.Children()[2];
                HE_EXPECT_EQ(style.Name(), "meta");
                HE_EXPECT_EQ(style.Properties().Size(), 2);
                HE_EXPECT_EQ(style.Properties().Get("name").String(), "description");
                HE_EXPECT_EQ(style.Properties().Get("content").String(), "kdl is a document language, mostly based on SDLang, with xml-like semantics that looks like you're invoking a bunch of CLI commands!");
            }
            {
                const KdlNode& script = head.Children()[3];
                HE_EXPECT_EQ(script.Name(), "title");
                HE_EXPECT_EQ(script.Arguments().Size(), 1);
                HE_EXPECT_EQ(script.Arguments()[0].String(), "kdl - The KDL Document Language");
            }
            {
                const KdlNode& link = head.Children()[4];
                HE_EXPECT_EQ(link.Name(), "link");
                HE_EXPECT_EQ(link.Properties().Size(), 2);
                HE_EXPECT_EQ(link.Properties().Get("rel").String(), "stylesheet");
                HE_EXPECT_EQ(link.Properties().Get("href").String(), "/styles/global.css");
            }
        }
        {
            const KdlNode& body = node.Children()[1];
            HE_EXPECT_EQ(body.Name(), "body");
            HE_EXPECT_EQ(body.Children().Size(), 1);
            {
                const KdlNode& main = body.Children()[0];
                HE_EXPECT_EQ(main.Name(), "main");
                HE_EXPECT_EQ(main.Children().Size(), 4);
                {
                    const KdlNode& header = main.Children()[0];
                    HE_EXPECT_EQ(header.Name(), "header");
                    HE_EXPECT_EQ(header.Properties().Size(), 1);
                    HE_EXPECT_EQ(header.Properties().Get("class").String(), "py-10 bg-gray-300");
                    HE_EXPECT_EQ(header.Children().Size(), 1);
                    {
                        const KdlNode& h1 = header.Children()[0];
                        HE_EXPECT_EQ(h1.Name(), "h1");
                        HE_EXPECT_EQ(h1.Properties().Size(), 1);
                        HE_EXPECT_EQ(h1.Properties().Get("class").String(), "text-4xl text-center");
                        HE_EXPECT_EQ(h1.Arguments().Size(), 1);
                        HE_EXPECT_EQ(h1.Arguments()[0].String(), "kdl - The KDL Document Language");
                    }
                }
                {
                    const KdlNode& section = main.Children()[1];
                    HE_EXPECT_EQ(section.Name(), "section");
                    HE_EXPECT_EQ(section.Properties().Size(), 2);
                    HE_EXPECT_EQ(section.Properties().Get("class").String(), "kdl-section");
                    HE_EXPECT_EQ(section.Properties().Get("id").String(), "description");
                    HE_EXPECT_EQ(section.Children().Size(), 2);
                    {
                        const KdlNode& p = section.Children()[0];
                        HE_EXPECT_EQ(p.Name(), "p");
                        HE_EXPECT_EQ(p.Children().Size(), 3);
                        {
                            const KdlNode& dash = p.Children()[0];
                            HE_EXPECT_EQ(dash.Name(), "-");
                            HE_EXPECT_EQ(dash.Arguments().Size(), 1);
                            HE_EXPECT_EQ(dash.Arguments()[0].String(), "kdl is a document language, mostly based on ");
                        }
                        {
                            const KdlNode& link = p.Children()[1];
                            HE_EXPECT_EQ(link.Name(), "a");
                            HE_EXPECT_EQ(link.Properties().Size(), 1);
                            HE_EXPECT_EQ(link.Properties().Get("href").String(), "https://sdlang.org");
                            HE_EXPECT_EQ(link.Arguments().Size(), 1);
                            HE_EXPECT_EQ(link.Arguments()[0].String(), "SDLang");
                        }
                        {
                            const KdlNode& dash = p.Children()[2];
                            HE_EXPECT_EQ(dash.Name(), "-");
                            HE_EXPECT_EQ(dash.Arguments().Size(), 1);
                            HE_EXPECT_EQ(dash.Arguments()[0].String(), " with xml-like semantics that looks like you're invoking a bunch of CLI commands");
                        }
                    }
                    {
                        const KdlNode& p = section.Children()[1];
                        HE_EXPECT_EQ(p.Name(), "p");
                        HE_EXPECT_EQ(p.Arguments().Size(), 1);
                        HE_EXPECT_EQ(p.Arguments()[0].String(), "It's meant to be used both as a serialization format and a configuration language, and is relatively light on syntax compared to XML.");
                    }
                }
                {
                    const KdlNode& section = main.Children()[2];
                    HE_EXPECT_EQ(section.Name(), "section");
                    HE_EXPECT_EQ(section.Properties().Size(), 2);
                    HE_EXPECT_EQ(section.Properties().Get("class").String(), "kdl-section");
                    HE_EXPECT_EQ(section.Properties().Get("id").String(), "design-and-discussion");
                    HE_EXPECT_EQ(section.Children().Size(), 2);
                    {
                        const KdlNode& h2 = section.Children()[0];
                        HE_EXPECT_EQ(h2.Name(), "h2");
                        HE_EXPECT_EQ(h2.Arguments().Size(), 1);
                        HE_EXPECT_EQ(h2.Arguments()[0].String(), "Design and Discussion");
                    }
                    {
                        const KdlNode& p = section.Children()[1];
                        HE_EXPECT_EQ(p.Name(), "p");
                        HE_EXPECT_EQ(p.Children().Size(), 3);
                        {
                            const KdlNode& dash = p.Children()[0];
                            HE_EXPECT_EQ(dash.Name(), "-");
                            HE_EXPECT_EQ(dash.Arguments().Size(), 1);
                            HE_EXPECT_EQ(dash.Arguments()[0].String(), "kdl is still extremely new, and discussion about the format should happen over on the ");
                        }
                        {
                            const KdlNode& link = p.Children()[1];
                            HE_EXPECT_EQ(link.Name(), "a");
                            HE_EXPECT_EQ(link.Properties().Size(), 1);
                            HE_EXPECT_EQ(link.Properties().Get("href").String(), "https://github.com/kdoclang/kdl/discussions");
                            HE_EXPECT_EQ(link.Children().Size(), 1);
                            {
                                const KdlNode& dash = link.Children()[0];
                                HE_EXPECT_EQ(dash.Name(), "-");
                                HE_EXPECT_EQ(dash.Arguments().Size(), 1);
                                HE_EXPECT_EQ(dash.Arguments()[0].String(), "discussions");
                            }
                        }
                        {
                            const KdlNode& dash = p.Children()[2];
                            HE_EXPECT_EQ(dash.Name(), "-");
                            HE_EXPECT_EQ(dash.Arguments().Size(), 1);
                            HE_EXPECT_EQ(dash.Arguments()[0].String(), " page in the Github repo. Feel free to jump in and give us your 2 cents!");

                        }
                    }
                }
                {
                    const KdlNode& section = main.Children()[3];
                    HE_EXPECT_EQ(section.Name(), "section");
                    HE_EXPECT_EQ(section.Properties().Size(), 2);
                    HE_EXPECT_EQ(section.Properties().Get("class").String(), "kdl-section");
                    HE_EXPECT_EQ(section.Properties().Get("id").String(), "design-principles");
                    HE_EXPECT_EQ(section.Children().Size(), 2);
                    {
                        const KdlNode& h2 = section.Children()[0];
                        HE_EXPECT_EQ(h2.Name(), "h2");
                        HE_EXPECT_EQ(h2.Arguments().Size(), 1);
                        HE_EXPECT_EQ(h2.Arguments()[0].String(), "Design Principles");
                    }
                    {
                        const KdlNode& ol = section.Children()[1];
                        HE_EXPECT_EQ(ol.Name(), "ol");
                        HE_EXPECT_EQ(ol.Children().Size(), 5);
                        {
                            const KdlNode& li = ol.Children()[0];
                            HE_EXPECT_EQ(li.Name(), "li");
                            HE_EXPECT_EQ(li.Arguments().Size(), 1);
                            HE_EXPECT_EQ(li.Arguments()[0].String(), "Maintainability");
                        }
                        {
                            const KdlNode& li = ol.Children()[1];
                            HE_EXPECT_EQ(li.Name(), "li");
                            HE_EXPECT_EQ(li.Arguments().Size(), 1);
                            HE_EXPECT_EQ(li.Arguments()[0].String(), "Flexibility");
                        }
                        {
                            const KdlNode& li = ol.Children()[2];
                            HE_EXPECT_EQ(li.Name(), "li");
                            HE_EXPECT_EQ(li.Arguments().Size(), 1);
                            HE_EXPECT_EQ(li.Arguments()[0].String(), "Cognitive simplicity and Learnability");
                        }
                        {
                            const KdlNode& li = ol.Children()[3];
                            HE_EXPECT_EQ(li.Name(), "li");
                            HE_EXPECT_EQ(li.Arguments().Size(), 1);
                            HE_EXPECT_EQ(li.Arguments()[0].String(), "Ease of de/serialization");
                        }
                        {
                            const KdlNode& li = ol.Children()[4];
                            HE_EXPECT_EQ(li.Name(), "li");
                            HE_EXPECT_EQ(li.Arguments().Size(), 1);
                            HE_EXPECT_EQ(li.Arguments()[0].String(), "Ease of implementation");
                        }
                    }
                }
            }
        }
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, kdl_document, Write)
{
    KdlDocument doc;

    KdlNode& a = doc.Nodes().EmplaceBack("a");
    a.Arguments().EmplaceBack(true);

    KdlNode& b = doc.Nodes().EmplaceBack("b");
    b.Arguments().EmplaceBack(42);

    KdlNode& c = doc.Nodes().EmplaceBack("c");
    KdlNode& d = c.Children().EmplaceBack("d");
    d.Arguments().EmplaceBack("yes");

    KdlNode& e = doc.Nodes().EmplaceBack("e");
    e.Properties().Emplace("f0", true);
    e.Properties().Emplace("f1", false);

    const String output = doc.ToString();
    HE_EXPECT_EQ(output, "a #true\nb 42\nc {\n    d yes\n}\ne f0=#true f1=#false\n");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, kdl_document, spec_test_cases)
{
    const StringView filePath = HE_FILE;
    const StringView testDir = GetDirectory(filePath);

    String inputDir = testDir;
    ConcatPath(inputDir, "fixtures/kdl/test_cases/input");

    String expectedDir = testDir;
    ConcatPath(expectedDir, "fixtures/kdl/test_cases/expected_kdl");

    struct TestCase
    {
        String input;
        String expected;
    };
    HashMap<String, TestCase> testCases;

    // Read all the input and expected files for the KDL tests
    DirectoryScanner scanner;
    Result r = scanner.Open(inputDir.Data());
    HE_EXPECT(r, r);

    DirectoryScanner::Entry entry;
    while (scanner.NextEntry(entry))
    {
        if (entry.isDirectory)
            continue;

        if (GetExtension(entry.name) != ".kdl")
            continue;

        TestCase& test = testCases.Emplace(entry.name).entry.value;

        String path = inputDir;
        ConcatPath(path, entry.name);
        r = File::ReadAll(test.input, path.Data());
        HE_EXPECT(r, r);

        path = expectedDir;
        ConcatPath(path, entry.name);
        r = File::ReadAll(test.expected, path.Data());

        // Expected file may not exist, this is OK. It means that the input should fail to parse.
        if (GetFileResult(r) != FileResult::NotFound)
        {
            HE_EXPECT(r, r);
        }
    }

    // Test each file by round tripping the input and checking it against the expected output.
    for (const auto& test : testCases)
    {
        const String& key = test.key;
        const String& input = test.value.input;
        const String& expected = test.value.expected;

        KdlDocument doc;
        const KdlReadResult rc = doc.Read(input);

        // We don't support the out-of-range number values in these test cases
        if (key == "hex_int.kdl"
            || key == "sci_notation_large.kdl"
            || key == "sci_notation_small.kdl")
        {
            HE_EXPECT(!rc.IsValid(), key);
            HE_EXPECT_EQ(rc.error, KdlReadError::InvalidNumber, key);
            continue;
        }

        // I'm pretty sure this test case is incorrect and it should actually fail.
        // There is a string in here that seems to have a dedent prefix that isn't honored,
        // the harvest parser fails which I believe is correct.
        if (key == "multiline_string_whitespace_only.kdl")
        {
            HE_EXPECT(!rc.IsValid(), key);
            HE_EXPECT_EQ(rc.error, KdlReadError::InvalidToken, key);
            continue;
        }

        if (expected.IsEmpty())
        {
            HE_EXPECT(!rc.IsValid(), key);
            continue;
        }

        HE_EXPECT(rc.IsValid(), key, rc.error, rc.line, rc.column);

        String output;
        doc.Write(output);

        // Our default float fmt cuts off the exponent at 16 precision, not 10.
        // So "node 1e10" will print as the full 10 digits.
        if (key == "no_decimal_exponent.kdl"
            || key == "parse_all_arg_types.kdl"
            || key == "positive_exponent.kdl"
            || key == "prop_float_type.kdl")
        {
            String expected2 = expected;
            expected2.Replace("1.0E+10", "10000000000.0");
            expected2.Replace("1E+10", "10000000000.0");
            expected2.Replace("2.5E+10", "25000000000.0");
            HE_EXPECT(output == expected2, key, output, expected2);
        }
        else
        {
            HE_EXPECT_EQ(output, expected, key);
        }
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, kdl_document, spec_benchmarks)
{
    // These run pretty slowly in debug
#if !HE_CFG_DEBUG
    auto runBench = [](const StringView filename)
    {
        const String inputPath = String("fixtures/kdl/benchmarks/") + filename;
        const String input = ReadFixtureFile(inputPath.Data());

        KdlDocument doc;
        const Stopwatch sw;
        const KdlReadResult rc = doc.Read(input);
        const Duration elapsed = sw.Elapsed();

        HE_EXPECT(rc.IsValid(), filename, rc.error, rc.line, rc.column);
        HE_LOGF_INFO(he_test, "    Parsed {} in {} ms", filename, ToPeriod<Milliseconds>(elapsed));
    };

    runBench("html-standard.kdl");
    runBench("html-standard-compact.kdl");
#endif
}
