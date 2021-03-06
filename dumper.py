import re

class Node:
  def __init__(self):
    self.children = {}
  def __str__(self):
    return "%d %d" % (self.result, self.count)

class Tree:
  def __init__(self, n, d, root):
    self.n = n
    self.d = d
    self.root = root

parser = re.compile(r"(\d+) (\d+) (\d+) :(.*)$")

def read_line(f, node):
  line = f.readline()
  p = parser.match(line)
  (result, count, size, children) = [p.group(x) for x in range(1, 5)]  
  node.result = int(result)
  node.count = int(count)
  for childpos in map(int, children.split()):
    child_node = Node()
    read_line(f, child_node)
    node.children[childpos] = child_node

def read_file(filename):
  f = open(filename, "rt")
  n, d = map(int, f.readline().split())
  root = Node()
  read_line(f, root)
  return Tree(n, d, root)

def encode(value):
  d = {0: "X wins", 1: "O wins", 2: "draw"}
  return d[value]

def print_board(n, d, board, node):
  if d == 2:
    x_set = set(p for i, p in enumerate(board) if i % 2 == 0)
    o_set = set(p for i, p in enumerate(board) if i % 2 == 1)
    s = ['<p>Result: %s</p><table class="border board">' % encode(node.result)]
    children = node.children.keys()
    for j in range(0, n):
      s.append('<tr class="border">')
      for i in range(0, n):
        s.append('<td class="border cell">')
        pos = j * n + i
        if pos in x_set:
          s.append("X")
        elif pos in o_set:
          s.append("O")
        elif pos in children:
          s.append('<a href="sffdf">%d</a><br>(%s)' %
              (node.children[pos].count, encode(node.children[pos].result)))
        else:
          s.append("&nbsp;")
        s.append("</td>")
      s.append("</tr>")
    s.append("</table><br><br>")
    return "".join(s)

def top_html():
  return """
  <html>
  <head>
  <style>
    .board {
      border-collapse: collapse;
    }
    .border {
      border: 1px solid black;
    }
    .cell {
      width: 60px;
      height: 60px;
    }
  </style>
  </head>
  <body>"""

def bottom_html():
  return """
  </body>
  </html>
  """

def print_tree(tree, node, board, max_depth, depth):
  print(print_board(tree.n, tree.d, board, node))
  if depth < max_depth:
    for pos, childnode in node.children.items():
      board.append(pos)
      print_tree(tree, childnode, board, max_depth, depth + 1)
      board.pop()

def main():
  tree = read_file("solution.txt")
  print(top_html())
  print_tree(tree, tree.root, [], 5, 0)
  print(bottom_html())

if __name__ == '__main__':
  main()

