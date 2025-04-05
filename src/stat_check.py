import matplotlib.pyplot as plt
import sys

def read_data(filename):
    iterations = []
    cycles = []
    with open(filename, 'r') as f:
        for line in f:
            parts = line.strip().split()
            if len(parts) == 2:
                iterations.append(int(parts[0]))
                cycles.append(int(parts[1]))
    return iterations, cycles

def plot_data(filenames, output_filename):
    plt.figure(figsize=(10, 6))
    
    for filename in filenames:
        iterations, cycles = read_data(filename)
        filename = filename[9:]
        filename = filename[:-5]
        if iterations and cycles:
            plt.plot(iterations, cycles, marker='o', linestyle='-', label=filename)
    
    plt.xlabel('Номер итерации')
    plt.ylabel('Количество тактов')
    plt.title('Зависимость времени выполнения от номера итерации')
    plt.grid(True)
    plt.legend(bbox_to_anchor=(1.05, 1), loc='upper left')
    plt.tight_layout()
    
    plt.savefig(output_filename, bbox_inches='tight', dpi=600)
    print(f"График сохранён как {output_filename}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Использование: python script.py input1.txt input2.txt ... output.png")
        sys.exit(1)
    
    filenames = sys.argv[1:-1]
    output_filename = sys.argv[-1]
    plot_data(filenames, output_filename)