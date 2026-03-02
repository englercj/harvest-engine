// Copyright Chad Engler

using Harvest.Make.Projects.Nodes;
using System.Diagnostics.CodeAnalysis;
using static Harvest.Make.Projects.ModuleGroupTree;

namespace Harvest.Make.Projects;

internal class ModuleGroupTree
{
    private static readonly Guid _namespaceProjectId = new("77D82CDB-779C-4088-925A-D97DFC780CBB");

    private static string GetModuleGuid(string path)
    {
        Guid value = GuidExtensions.CreateV5(_namespaceProjectId, path);
        string valueStr = value.ToString().ToUpperInvariant();
        return $"{{{valueStr}}}";
    }

    public static string GetModuleGuid(ModuleNode module)
    {
        string path = GetModulePath(module);
        return GetModuleGuid(path);
    }

    public static string GetModulePath(ModuleNode module)
    {
        return $"{module.Group ?? ""}/{module.ModuleName}";
    }

    internal class Entry(string path)
    {
        public string FullPath => path;
        public string Name { get; } = Path.GetFileName(path);
        public string ID { get; } = GetModuleGuid(path);
        public List<Entry> Children { get; } = [];
        public Entry? Parent { get; set; } = null;
        public bool HasModuleChildren { get; private set; } = false;

        public bool TryAddChild(Entry child)
        {
            if (Children.Find(e => e.Name == child.Name) is null)
            {
                Children.Add(child);
                child.Parent = this;

                HasModuleChildren |= child is ModuleEntry;

                return true;
            }

            return false;
        }
    }

    internal class ModuleEntry(ModuleNode module) : Entry(GetModulePath(module))
    {
        public EModuleLanguage Language => module.Language;
    }

    public Entry Root { get; } = new("<root>");

    private readonly Dictionary<string, ModuleEntry> _moduleEntriesByName = [];
    public IReadOnlyDictionary<string, ModuleEntry> ModuleEntriesByName => _moduleEntriesByName;

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

    public IEnumerable<Entry> Entries => EnumerateEntriesRecusive(Root);

    public bool TryAdd(ModuleNode module)
    {
        ModuleEntry child = new(module);
        Entry parent = FindOrCreateBranch(module.Group);
        if (parent.TryAddChild(child))
        {
            _moduleEntriesByName.Add(module.ModuleName, child);
            return true;
        }

        return false;
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

        Entry branch = new(path);
        parent.TryAddChild(branch);
        return branch;
    }

    private static IEnumerable<Entry> EnumerateEntriesRecusive(Entry entry)
    {
        foreach (Entry child in entry.Children)
        {
            yield return child;
            foreach (Entry grandChild in EnumerateEntriesRecusive(child))
            {
                yield return grandChild;
            }
        }
    }
}
