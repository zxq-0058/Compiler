import networkx as nx
import matplotlib.pyplot as plt

# 从文件读取内容
def read_blocks(filename):
    with open(filename, 'r') as file:
        lines = file.readlines()
    return [line.strip() for line in lines]

# 构建有向图
def build_graph(output):
    G = nx.MultiDiGraph()
    current_block = None

    for line in output:
        if line.startswith("Basic Block"):
            block_num = int(line.split()[2][:-1])
            G.add_node(block_num)
            current_block = block_num
        elif line.startswith("successors") and current_block is not None:
            if line == "successors:":
                continue
            succ_str = line.split(": ")[1]
            if succ_str.strip() != "":
                successors = succ_str.split()
                for succ in successors:
                    G.add_edge(current_block, int(succ))

    return G

# 绘制有向图并导出到图片
def export_graph(G, filename):
    plt.figure(figsize=(8, 6))
    pos = nx.spring_layout(G)
    edge_labels = nx.get_edge_attributes(G, 'key')
    nx.draw_networkx(G, pos, with_labels=True, node_size=1000, node_color='lightblue', edge_color='gray', arrows=True)
    nx.draw_networkx_edge_labels(G, pos, edge_labels=edge_labels, font_color='red')
    plt.savefig(filename)
    plt.show()

# 主函数
def main():
    filename = "flow_graph.txt"  # 文件名
    output_filename = "graph.png"  # 输出图片文件名

    # 读取文件内容
    output = read_blocks(filename)

    # 构建有向图
    G = build_graph(output)

    # 导出有向图到图片
    export_graph(G, output_filename)

if __name__ == '__main__':
    main()
