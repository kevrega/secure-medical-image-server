from sklearn.cluster import KMeans

import pydicom
import matplotlib.pyplot as plt
import numpy as np

from pydicom.data import get_testdata_file


# Find the DICOM file
file_path = get_testdata_file("CT_small.dcm")

# Read the DICOM file
dataset = pydicom.dcmread(file_path)

# Get the actual image pixels
image = dataset.pixel_array

print("Modality:", dataset.Modality)
print("Image size:", image.shape)
print("Lowest pixel value:", image.min())
print("Highest pixel value:", image.max())

# Save original CT image
plt.imshow(image, cmap="gray")
plt.savefig("ct_image.png")
plt.clf()

# Make ONE long column of pixels
# -1 means Python calculates how many rows are needed
pixels = image.reshape(-1, 1)

print("Pixels shape:", pixels.shape)

# Create K-Means ML model and divide pixels into 3 groups
model = KMeans(n_clusters=3, random_state=0)

# Let K-Means learn the 3 groups from the pixel values
model.fit(pixels)

# Get which group (0, 1, or 2) each pixel belongs to
labels = model.labels_

print("First 20 labels:", labels[:20])

# Show the average/center pixel value of each group
print("Cluster centers:", model.cluster_centers_)

# Count how many pixels belong to each group
unique, counts = np.unique(labels, return_counts=True)

print("Labels:", unique)
print("Counts:", counts)

# Turn the long list of labels back into a 128x128 image
clustered_image = labels.reshape(image.shape)

# Save the clustered result
plt.imshow(clustered_image)
plt.savefig("clustered_ct.png")

# Save ML result so the C++ server can read it
with open("data/analysis_result.txt", "w") as file:
    file.write("Cluster centers: ")
    file.write(str(model.cluster_centers_.flatten())) # Make the cluster centre into a long row instead
    file.write("\n")

    file.write("Counts: ")
    file.write(str(counts))
    file.write("\n")