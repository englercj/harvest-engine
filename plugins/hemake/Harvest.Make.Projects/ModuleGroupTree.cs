// Copyright Chad Engler

using Harvest.Make.Projects.Nodes;
using System.Diagnostics.CodeAnalysis;

namespace Harvest.Make.Projects;

public class ModuleGroupTree
{
    private static readonly Guid _namespaceProjectId = new("77D82CDB-779C-4088-925A-D97DFC780CBB");

    private static string GetModuleGuid(string path)
    {
        Guid value = GuidUtils.CreateV5(_namespaceProjectId, path);
        string valueStr = value.ToString().ToUpperInvariant();
        return $"{{{valueStr}}}";
    }

    public static string GetModuleGuid(ModuleNode module)
    {
        string path = $"{module.Group ?? ""}/{module.ModuleName}";
        return GetModuleGuid(path);
    }

    public class Entry(string path, ModuleNode? module)
    {
        public string FullPath => path;
        public string Name { get; } = Path.GetFileName(path);
        public string ID { get; } = GetModuleGuid(path);
        public ModuleNode? Module => module;
        public List<Entry> Children { get; } = [];
        public Entry? Parent { get; set; }
    }

    private readonly Dictionary<string, Entry> _modulesByName = [];

    public Entry Root { get; } = new("<root>", null);

    public bool HasBranches
    {
        get
        {
            foreach (Entry entry in Root.Children)
            {
                if (entry.Children.Count > 0)
                {
                    return true;
                }
            }

            return false;
        }
    }

    public IEnumerable<Entry> Entries => EnumerateChildrenRecursive(Root);

    public Entry Add(ModuleNode module)
    {
        string path = $"{module.Group ?? ""}/{module.ModuleName}";
        Entry newEntry = new(path, module);
        Entry parent = FindOrCreateBranch(module.Group);
        Insert(newEntry, parent);
        _modulesByName.Add(module.ModuleName, newEntry);
        return newEntry;
    }

    public void Clear()
    {
        Root.Children.Clear();
    }

    public void Sort()
    {
        Sort((a, b) => string.Compare(a.Name, b.Name));
    }

    public void Sort(Comparison<Entry> compare)
    {
        foreach (Entry child in Entries)
        {
            child.Children.Sort(compare);
        }
    }

    public bool TryGetEntry(string name, [MaybeNullWhen(false)] out Entry entry)
    {
        return _modulesByName.TryGetValue(name, out entry);
    }

    private Entry FindOrCreateBranch(string? path)
    {
        if (string.IsNullOrEmpty(path) || path == "." || path == "/")
        {
            return Root;
        }

        Entry parent = FindOrCreateBranch(Path.GetDirectoryName(path));

        foreach (Entry child in parent.Children)
        {
            if (child.FullPath == path)
            {
                return child;
            }
        }

        Entry branch = new(path, null);
        Insert(branch, parent);
        return branch;
    }

    private static void Insert(Entry entry, Entry parent)
    {
        entry.Parent = parent;
        parent.Children.Add(entry);
    }

    private static IEnumerable<Entry> EnumerateChildrenRecursive(Entry entry)
    {
        foreach (Entry child in entry.Children)
        {
            yield return child;

            foreach (Entry grandChild in EnumerateChildrenRecursive(child))
            {
                yield return grandChild;
            }
        }
    }
}
