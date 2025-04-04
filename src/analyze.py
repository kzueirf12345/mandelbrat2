import sys
import matplotlib.pyplot as plt
import numpy as np
from scipy import stats

def process_file(filename, repeats):
    iterations = []
    times = []
    
    with open(filename, 'r') as file:
        for line in file:
            parts = line.strip().split()
            if len(parts) >= 2:
                iteration = int(parts[0])
                time = float(parts[1])
                iterations.append(iteration)
                times.append(time / repeats)
    
    if not times:
        return None, None
    
    mean_time = np.mean(times)
    sem = stats.sem(times) 
    
    return mean_time, sem

def plot_comparison(input_files, output_file, measurements, repeats):
    averages = []
    sems = []
    
    for file in input_files:
        mean_time, sem = process_file(file, repeats)
        if mean_time is None:
            continue
            
        averages.append(1/mean_time)
        sems.append(sem)
    

    ratios = []
    if len(averages) > 1:
        base = min(averages)
        ratios = [f"{avg/base:.2f}x" for avg in averages]
    

    plt.figure(figsize=(12, 8))
    x = np.arange(len(input_files))
    bars = plt.bar(x, averages, width=0.6, 
                  #yerr=sems,
                  capsize=5,
                  color=plt.cm.tab20.colors[:len(input_files)])
    

    for i, (bar, ratio) in enumerate(zip(bars, ratios)):
        height = bar.get_height()
        plt.text(bar.get_x() + bar.get_width()/2., height + 0.01*max(averages),
                f'{ratio}\n{averages[i]:.5f} ± {sems[i]:.5f} * 10⁹',
                ha='center', va='bottom', fontsize=9)
    
    plt.xticks(x, input_files, rotation=45, ha='right')
    plt.xlabel('Версии программы')
    plt.ylabel('Среднее время на 1 кадр (в тактах)')
    
    title = f'Сравнение производительности\n'
    title += f'Измерений: {measurements}, Повторов: {repeats}'
    plt.title(title)
    
    plt.tight_layout()
    
    plt.savefig(output_file, bbox_inches='tight', dpi=600)
    print(f"График сохранён как {output_file}")

if __name__ == "__main__":
    if len(sys.argv) < 5:
        print("Использование: python script.py measurements repeats input1.txt input2.txt ... output.png")
        sys.exit(1)
    
    measurements = int(sys.argv[1])
    repeats = int(sys.argv[2])
    input_files = sys.argv[3:-1]
    output_file = sys.argv[-1]
    
    plot_comparison(input_files, output_file, measurements, repeats)