// Copyright Chad Engler

namespace Harvest.Make.Projects;

public class WhenExpressionEvaluator
{
    static readonly HashSet<char> s_nonLiteralCharacters = ['!', '(', ')', '&', '|', '^'];

    public delegate bool EvaluateValueDelegate(string value);

    private class EvaluationContext(EvaluateValueDelegate evaluator)
    {
        public EvaluateValueDelegate Evaluator => evaluator;
    }

    private abstract class TreeNode
    {
        public virtual int Arity => 0;
        public TreeNode? Left { get; set; }
        public TreeNode? Right { get; set; }
        public abstract bool Evaluate(EvaluationContext context);
    }

    private class LiteralNode(string value) : TreeNode
    {
        public string Value => value;
        public override int Arity => 0;

        public override bool Evaluate(EvaluationContext context)
        {
            return context.Evaluator(value);
        }
    }

    private class NotNode : TreeNode
    {
        public override int Arity => 1;

        public override bool Evaluate(EvaluationContext context)
        {
            if (Left is null)
            {
                throw new Exception("Invalid expression. NOT (!) operator must have a target.");
            }

            return !Left.Evaluate(context);
        }
    }

    private class AndNode : TreeNode
    {
        public override int Arity => 2;

        public override bool Evaluate(EvaluationContext context)
        {
            if (Left is null || Right is null)
            {
                throw new Exception("Invalid expression. AND (&&) operator must have a left and right expression.");
            }

            return Left.Evaluate(context) && Right.Evaluate(context);
        }
    }

    private class OrNode : TreeNode
    {
        public override int Arity => 2;

        public override bool Evaluate(EvaluationContext context)
        {
            if (Left is null || Right is null)
            {
                throw new Exception("Invalid expression. OR (||) operator must have a left and right expression.");
            }

            return Left.Evaluate(context) || Right.Evaluate(context);
        }
    }

    private class XorNode : TreeNode
    {
        public override int Arity => 2;

        public override bool Evaluate(EvaluationContext context)
        {
            if (Left is null || Right is null)
            {
                throw new Exception("Invalid expression. XOR (^) operator must have a left and right expression.");
            }

            return Left.Evaluate(context) ^ Right.Evaluate(context);
        }
    }

    public static bool Evaluate(string expression, EvaluateValueDelegate evaluator)
    {
        TreeNode? root = ParseExpression(expression);

        if (root is null)
            return true;

        EvaluationContext context = new(evaluator);
        return root.Evaluate(context);
    }

    // Parse the expression string into a tree of nodes. The expression can support grouping by parentheses.
    // The expression can contain the following operators:
    //   - NOT (!)
    //   - AND (&&)
    //   - OR (||)
    //   - XOR (^)
    // The expression can contain variables that are replaced with values from the variables dictionary.
    private static TreeNode? ParseExpression(string expression)
    {
        if (string.IsNullOrEmpty(expression))
            return null;

        Stack<TreeNode> stack = new();

        for (int i = 0; i < expression.Length; ++i)
        {
            char c = expression[i];

            if (char.IsWhiteSpace(c))
                continue;

            // (a && b) || (c && d)
            //          OR
            //        /    \
            //    AND       AND
            //   /   \     /   \
            //  a     b   c     d

            switch (c)
            {
                case '!':
                {
                    stack.Push(new NotNode());
                    break;
                }

                case '&':
                {
                    if (expression.Length <= i + 1 || expression[i + 1] != '&')
                    {
                        throw new Exception("Invalid expression. Unexpected token '&'. Did you mean to use '&&'?");
                    }

                    if (stack.Count == 0)
                    {
                        throw new Exception("Invalid expression. AND (&&) operators must have an expression to their left.");
                    }

                    ++i;
                    stack.Push(new AndNode()
                    {
                        Left = stack.Pop()
                    });
                    break;
                }

                case '|':
                {
                    if (expression.Length <= i + 1 || expression[i + 1] != '|')
                    {
                        throw new Exception("Invalid expression. Unexpected token '|'. Did you mean to use '||'?");
                    }

                    if (stack.Count == 0)
                    {
                        throw new Exception("Invalid expression. OR (||) operators must have an expression to their left.");
                    }

                    ++i;
                    stack.Push(new OrNode()
                    {
                        Left = stack.Pop()
                    });
                    break;
                }

                case '^':
                {
                    if (stack.Count == 0)
                    {
                        throw new Exception("Invalid expression. XOR (^) operators must have an expression to their left.");
                    }

                    stack.Push(new XorNode()
                    {
                        Left = stack.Pop()
                    });
                    break;
                }

                case '(':
                {
                    if (ParseSubExpression(expression, ref i) is TreeNode node)
                    {
                        AddNodeToParentOrStack(node, stack);
                    }
                    break;
                }

                case ')':
                {
                    throw new Exception("Invalid expression. Unexpected token ')'.");
                }

                default:
                {
                    LiteralNode node = ParseLiteral(expression, ref i);
                    AddNodeToParentOrStack(node, stack);
                    break;
                }
            }
        }

        return stack.Count == 1 ? stack.Pop() : null;
    }

    private static LiteralNode ParseLiteral(string expression, ref int index)
    {
        int start = index;
        while (index < expression.Length)
        {
            char c = expression[index];

            if (char.IsWhiteSpace(c) || s_nonLiteralCharacters.Contains(c))
                break;

            ++index;
        }

        return new LiteralNode(expression[start..index]);
    }

    private static TreeNode? ParseSubExpression(string expression, ref int index)
    {
        int parenCount = 0;
        int start = index;
        while (index < expression.Length)
        {
            char c = expression[index];

            if (c == '(')
                ++parenCount;
            else if (c == ')')
                --parenCount;

            if (parenCount == 0)
                break;

            ++index;
        }

        if (parenCount != 0)
        {
            throw new Exception("Invalid expression. Mismatched parentheses.");
        }

        ++start; // skip the opening parenthesis
        string subExpression = expression[start..index];
        ++index; // skip the closing parenthesis

        return ParseExpression(subExpression);
    }

    private static void AddNodeToParentOrStack(TreeNode node, Stack<TreeNode> stack)
    {
        if (stack.Count > 0)
        {
            TreeNode parent = stack.Peek();
            if (parent.Left is null && parent.Arity >= 1)
            {
                parent.Left = node;
            }
            else if (parent.Right is null && parent.Arity >= 2)
            {
                parent.Right = node;
            }
            else
            {
                throw new Exception("Invalid expression. Unexpected token.");
            }
        }
        else
        {
            stack.Push(node);
        }
    }
}
