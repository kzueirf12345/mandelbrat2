import sys
import matplotlib.pyplot as plt
import numpy as np
from scipy import stats
from math import log, sqrt

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
                times.append(1 / (time / repeats))
    
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
            
        averages.append(mean_time)
        sems.append(sem)
    

    ratios = []
    if len(averages) > 1:
        base = min(averages)
        base_sem = sems[np.argmin(averages)]
        ratios = [f"{averages[i]/base:.3f}x ± {averages[i]/base*sqrt((sems[i]/averages[i])**2 + (base_sem/base)**2):.3f}x" for i in range(len(averages))]
    

    plt.figure(figsize=(12, 8))
    x = np.arange(len(input_files))
    bars = plt.bar(x, averages, width=0.6, 
                  yerr=sems,
                  capsize=5,
                  color=plt.cm.tab20.colors[:len(input_files)])
    

    for i, (bar, ratio) in enumerate(zip(bars, ratios)):
        # height = bar.get_height()
        # plt.text(bar.get_x() + bar.get_width()/2., height + 0.01*max(averages),
        #         f'{ratio}',
        #         ha='center', va='bottom', fontsize=9)
        print(f'{input_files[i]}\n{ratio} & {1/averages[i]/1000_000:.4f} ± {sems[i]/averages[i]/averages[i]/1000_000:.4f}')
        
    nums = [i for i in range(len(input_files))];
    # for i in range(len(input_files)):
    #     input_files[i] = input_files[i][9:]
    #     input_files[i] = input_files[i][:-5]
        
    plt.xticks(x, nums, rotation=0, ha='right')
    plt.xlabel('Версии программы')
    plt.ylabel('Обратная величина ко времени выполнения 1 итерации (1/такт)')
    
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