from ctypes import *
from typing import List
from vaitrace_py import vai_tracepoint
import cv2
import numpy as np
import xir
import vart
import os
import math
import time
import sys

duration = 15
time_c_start = time.time()
@vai_tracepoint
def CPUCalcSoftmax(data, size, scale):
    sum = 0.0
    result = [0 for i in range(size)]
    for i in range(size):
        result[i] = math.exp(data[i] * scale)
        sum += result[i]
    for i in range(size):
        result[i] /= sum
    return result

def get_script_directory():
    path = os.getcwd()
    return path

def TopK(datain, size, filePath):
    if not datain or size == 0:
        print("数据为空或大小为0")
        return

    cnt = list(range(size))
    pair = sorted(zip(datain, cnt), reverse=True)

    try:
        softmax_new, cnt_new = zip(*pair)
    except ValueError:
        print("解包失败，可能是因为输入数据为空")
        return

    with open(filePath, "r") as fp:
        data1 = fp.readlines()

    for i in range(min(5, len(data1))):  # 确保不会超出data1的范围
        idx = cnt_new[i]
        if idx < len(data1):
            print(f"Top[{i}] {idx} {data1[idx].strip()}")
        else:
            print(f"Top[{i}] {idx} - 索引超出范围")

def preprocess_one_image_fn(image_path, fix_scale, width=240, height=240):
    means = [104.0, 107.0, 123.0]
    scales = [1.0, 1.0, 1.0]
    image = cv2.imread(image_path)
    image = cv2.resize(image, (width, height))
    B, G, R = cv2.split(image)
    B = (B - means[0]) * scales[0] * fix_scale
    G = (G - means[1]) * scales[1] * fix_scale
    R = (R - means[2]) * scales[2] * fix_scale
    image = cv2.merge([B, G, R])
    image = image.astype(np.int8)
    return image

SCRIPT_DIR = get_script_directory()
calib_image_dir = SCRIPT_DIR + "/../images/"

def runResnet50(runner, img, cnt, duration):
    inputTensors = runner.get_input_tensors()
    outputTensors = runner.get_output_tensors()
    input_ndim = tuple(inputTensors[0].dims)
    pre_output_size = int(outputTensors[0].get_data_size() / input_ndim[0])
    output_ndim = tuple(outputTensors[0].dims)
    output_fixpos = outputTensors[0].get_attr("fix_point")
    output_scale = 1 / (2**output_fixpos)
    n_of_images = len(img)
    count = 0
    dnn_name = os.path.splitext(os.path.basename(sys.argv[1]))[0]  # 从xmodel文件名提取dnn_name

    while count < cnt:
        runSize = min(cnt - count, input_ndim[0])
        inputData = [np.empty(input_ndim, dtype=np.int8, order="C")]
        outputData = [np.empty(output_ndim, dtype=np.int8, order="C")]
        for j in range(runSize):
            index = (count + j) % n_of_images
            inputData[0][j, ...] = img[index].reshape(input_ndim[1:])
       
        # 开始执行推理
        start_time = time.time()
        print(f"elapsed time: {start_time - time_c_start}")
        while time.time() - start_time < duration:
            job_id = runner.execute_async(inputData, outputData)
            runner.wait(job_id)
        
        end_time = time.time()
        print(f"推理执行完成，耗时：{end_time - start_time} 秒。")

        for j in range(runSize):
            softmax = CPUCalcSoftmax(outputData[0][j], pre_output_size, output_scale)
            TopK(softmax, pre_output_size, SCRIPT_DIR + "/words.txt")
        count += runSize

def get_child_subgraph_dpu(graph: "Graph") -> List["Subgraph"]:
    assert graph is not None, "'graph' should not be None."
    root_subgraph = graph.get_root_subgraph()
    if root_subgraph.is_leaf:
        return []
    child_subgraphs = root_subgraph.toposort_child_subgraph()
    return [cs for cs in child_subgraphs if cs.has_attr("device") and cs.get_attr("device").upper() == "DPU"]

def main(model_path, duration):
    listimage = os.listdir(calib_image_dir)
    runTotall = len(listimage)

    # 加载模型并获取DPU运行器
    g = xir.Graph.deserialize(model_path)
    subgraphs = get_child_subgraph_dpu(g)

    runner = vart.Runner.create_runner(subgraphs[0], "run")

    input_fixpos = runner.get_input_tensors()[0].get_attr("fix_point")
    input_scale = 2 ** input_fixpos

    img = []
    for image_name in listimage:
        path = os.path.join(calib_image_dir, image_name)
        img.append(preprocess_one_image_fn(path, input_scale))

    cnt = 1
    time_start = time.time()
    runResnet50(runner, img, cnt, duration)

    time_end = time.time()
    timetotal = time_end - time_start
    total_frames = cnt
    fps = float(total_frames / timetotal)
    print("FPS=%.2f, total frames = %.2f, time=%.6f seconds" % (fps, total_frames, timetotal))

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("使用方式: python3 script.py <resnet50_xmodel_file> <duration>")
    else:
        main(sys.argv[1], int(sys.argv[2]))
