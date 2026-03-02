// Copyright Chad Engler

using System.Text.RegularExpressions;

namespace Harvest.Make.Projects;

internal class StringTokenContext(Match match)
{
    public Match TokenMatch => match;
    public string Token => TokenMatch.Value;

    public Capture ContextNameCapture => TokenMatch.Groups["context"];
    public string ContextName => ContextNameCapture.Value;

    public Capture ContextIdCapture => TokenMatch.Groups["contextId"];
    public string ContextId => ContextIdCapture.Value;

    public Capture PropertyCapture => TokenMatch.Groups["property"];
    public string PropertyName => PropertyCapture.Value;

    public IReadOnlyList<Capture> TransformerCaptures => TokenMatch.Groups["transformer"].Captures;
    public IEnumerable<string> TransformerNames
    {
        get
        {
            foreach (Capture transformerCapture in TransformerCaptures)
            {
                // Each transformer capture includes the leading colon and possible whitespace, so trim those off.
                yield return transformerCapture.Value[1..].Trim();
            }
        }
    }
}

public interface IStringTokenHandler
{
    public string GetTokenValue(StringTokenContext tokenContext);
}

public partial class StringTokenReplacer(IStringTokenHandler handler)
{
    private readonly IStringTokenHandler _handler = handler;

    [GeneratedRegex(
        @"\$\{[ \t]*(?<context>\w+)[ \t]*(?:\[[ \t]*(?<contextId>[\.\w]+)[ \t]*\])?\.[ \t]*(?<property>\w+)[ \t]*(?<transformer>:[ \t]*\w+[ \t]*)*\}",
        RegexOptions.Singleline | RegexOptions.ExplicitCapture
    )]
    private static partial Regex TokenRegex();

    public string ReplaceTokens(string input)
    {
        return TokenRegex().Replace(input, (match) =>
        {
            StringTokenContext tokenContext = new(match);

            // ${context.property} is the minimal valid format.
            if (string.IsNullOrEmpty(tokenContext.ContextName) || string.IsNullOrEmpty(tokenContext.PropertyName))
            {
                throw new Exception($"Invalid token '{tokenContext.Token}'. Expected format: ${{context.property}}");
            }

            return _handler.GetTokenValue(tokenContext);
        });
    }
}
