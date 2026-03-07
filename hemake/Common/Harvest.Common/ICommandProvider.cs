using System.CommandLine;

namespace Harvest.Common;

public interface ICommandProvider
{
    Command GetCommand();
}
