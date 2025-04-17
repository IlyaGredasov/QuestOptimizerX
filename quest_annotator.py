import cv2
import sys
import math
import argparse
from pathlib import Path
import numpy as np
from ultralytics import YOLO

rectangles = []
drawing = False
start_point = (-1, -1)
current_rect = None
scale = 1.0
scale_step = 0.1
min_scale = 0.1
max_scale = 5.0
original_image = None
image_path = None

def point_in_rect(x, y, rect):
    x1, y1, x2, y2 = rect
    return x1 <= x <= x2 and y1 <= y <= y2

def mouse_callback(event, x, y, flags, param):
    global drawing, start_point, current_rect, rectangles

    x = int(x / scale)
    y = int(y / scale)

    if event == cv2.EVENT_LBUTTONDOWN:
        drawing = True
        start_point = (x, y)
        current_rect = None

    elif event == cv2.EVENT_MOUSEMOVE and drawing:
        current_rect = (start_point[0], start_point[1], x, y)

    elif event == cv2.EVENT_LBUTTONUP and drawing:
        drawing = False
        x1, y1 = start_point
        x2, y2 = x, y
        x1, x2 = sorted([x1, x2])
        y1, y2 = sorted([y1, y2])
        rectangles.append((x1, y1, x2, y2))
        current_rect = None

    elif event == cv2.EVENT_RBUTTONDOWN:
        for i, rect in enumerate(rectangles):
            if point_in_rect(x, y, rect):
                del rectangles[i]
                break

def draw_all(img, annotate_ids=False):
    for idx, rect in enumerate(rectangles):
        x1, y1, x2, y2 = rect
        cv2.rectangle(img, (x1, y1), (x2, y2), (0, 255, 0), 2)
        if annotate_ids:
            cx = int((x1 + x2) / 2)
            cv2.putText(img, str(idx), (cx, y1 - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 255), 2)

def generate_outputs(save_dir: Path, base_name: str, alpha: float):
    h, w = original_image.shape[:2]
    centers = []

    for rect in rectangles:
        x1, y1, x2, y2 = rect
        cx = (x1 + x2) / 2
        cy = (y1 + y2) / 2
        centers.append((cx, cy))

    n = len(centers)
    distances = [[0.0] * n for _ in range(n)]
    for i in range(n):
        for j in range(n):
            if i != j:
                x1, y1 = centers[i]
                x2, y2 = centers[j]
                distances[i][j] = math.hypot(x2 - x1, y2 - y1)

    threshold_edges = set()
    for i in range(n):
        d_min = min(distances[i][j] for j in range(n) if i != j)
        for j in range(n):
            if i < j and distances[i][j] <= alpha * d_min:
                threshold_edges.add((i, j, distances[i][j]))

    lines = [
        "FastTravel:\n\tFalse",
        "Bidirectional:\n\tTrue",
        "Weighted:\n\tTrue",
        f"VertexCount:\n\t{n}",
        "Edges:"
    ]
    for i, j, dist in sorted(threshold_edges):
        lines.append(f"\t{i} {j} {dist:.2f}")
    
    lines.append("QuestLines:")
    for i in range(n):
        lines.append(f"\t{i}")

    graph_file = save_dir / f"{base_name}_graph.txt"
    graph_file.write_text("\n".join(lines))

    labeled = original_image.copy()
    draw_all(labeled, annotate_ids=True)
    cv2.imwrite(str(save_dir / f"{base_name}_labeled.png"), labeled)

    print(f"Saved: {graph_file.name}, {base_name}_labeled.png")

def run_detection():
    global rectangles, original_image
    print("Running YOLOv8 detection...")
    model = YOLO("poi_detector.pt")
    results = model(original_image, imgsz=640, verbose=False)[0]

    for box in results.boxes.xyxy:
        x1, y1, x2, y2 = map(int, box.tolist())
        rectangles.append((x1, y1, x2, y2))
    print(f"Added {len(results.boxes)} rectangles from model")

def main():
    global original_image, scale, image_path

    parser = argparse.ArgumentParser()
    parser.add_argument("image_path", type=Path, help="Path to image file")
    parser.add_argument("--alpha", type=float, default=1.5, help="Relative edge distance threshold multiplier")
    args = parser.parse_args()

    image_path = args.image_path
    original_image = cv2.imread(str(image_path))
    if original_image is None:
        print(f"Could not load image: {image_path}")
        return

    base_name = image_path.stem
    save_dir = image_path.parent

    cv2.namedWindow("Annotator", cv2.WINDOW_NORMAL)
    cv2.setMouseCallback("Annotator", mouse_callback)

    h, w = original_image.shape[:2]
    cv2.resizeWindow("Annotator", int(w * scale), int(h * scale))

    while True:
        if cv2.getWindowProperty("Annotator", cv2.WND_PROP_VISIBLE) < 1:
            break

        resized = cv2.resize(original_image, (0, 0), fx=scale, fy=scale)
        display = resized.copy()
        draw_all(display)

        if current_rect:
            x1, y1, x2, y2 = current_rect
            cv2.rectangle(display, (int(x1 * scale), int(y1 * scale)), (int(x2 * scale), int(y2 * scale)), (0, 0, 255), 1)

        cv2.imshow("Annotator", display)
        key = cv2.waitKey(10) & 0xFF

        if key in [ord('+'), ord('=')]:
            scale = min(max_scale, scale + scale_step)
            cv2.resizeWindow("Annotator", int(w * scale), int(h * scale))
        elif key in [ord('-'), ord('_')]:
            scale = max(min_scale, scale - scale_step)
            cv2.resizeWindow("Annotator", int(w * scale), int(h * scale))
        elif key == ord('d'):
            run_detection()

    cv2.destroyAllWindows()
    generate_outputs(save_dir, base_name, alpha=args.alpha)

if __name__ == "__main__":
    main()
