// Copyright Chad Engler

using Harvest.Make.Projects.Nodes;
using System.Diagnostics.CodeAnalysis;

namespace Harvest.Make.Projects;

public class ModuleGroupTree
{
    private static readonly Guid _namespaceProjectId = new("77D82CDB-779C-4088-925A-D97DFC780CBB");

    public class Entry(string path, ModuleNode? module)
    {
        public string FullPath => path;
        public string Name { get; } = Path.GetFileName(path);
        public Guid ID { get; } = GuidUtils.CreateV5(_namespaceProjectId, path);
        public ModuleNode? Module => module;
        public List<Entry> Children { get; } = [];
        public Entry? Parent { get; set; }
    }

    private readonly Dictionary<string, Entry> _modulesByName = [];

    public Entry Root { get; } = new("", null);

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

    public Entry Add(ModuleNode module)
    {
        string path =$"{module.Group ?? ""}/{module.ModuleName}";
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
        Traverse((entry) => entry.Children.Sort(compare));
    }

    public bool TryGetEntry(string name, [MaybeNullWhen(false)] out Entry entry)
    {
        return _modulesByName.TryGetValue(name, out entry);
    }

    public void Traverse(Action<Entry> action)
    {
        TraverseChildren(Root, action);
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

    private static void TraverseChildren(Entry parent, Action<Entry> action)
    {
        foreach (Entry child in parent.Children)
        {
            action(child);

            if (child.Children.Count > 0)
            {
                TraverseChildren(child, action);
            }
        }
    }

    private static void Insert(Entry entry, Entry parent)
    {
        entry.Parent = parent;
        parent.Children.Add(entry);
    }
}
