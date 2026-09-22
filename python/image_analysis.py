import base64
import io
import json
import sys
from pathlib import Path

import numpy as np
import pydicom

from PIL import Image
from pydicom.data import get_testdata_file
from sklearn.cluster import KMeans


OUTPUT_FILE = Path("data/analysis_result.txt")


# Scale image values to 0-255 so they can be displayed
def normalize_image(image):

    image = image.astype(np.float32)

    minimum = float(np.min(image))
    maximum = float(np.max(image))

    if maximum == minimum:
        return np.zeros(
            image.shape,
            dtype=np.uint8
        )

    normalized = (
        image - minimum
    ) / (
        maximum - minimum
    )

    normalized = (
        normalized * 255
    ).clip(
        0,
        255
    )

    return normalized.astype(np.uint8)


# Convert a grayscale image to PNG and then Base64
def png_base64(image):

    image = normalize_image(image)

    picture = Image.fromarray(
        image,
        mode="L"
    )

    buffer = io.BytesIO()

    picture.save(
        buffer,
        format="PNG"
    )

    return base64.b64encode(
        buffer.getvalue()
    ).decode("ascii")


# Create a colored image showing the three K-Means groups
def clustered_png_base64(labels):

    palette = np.array(
        [
            [18, 32, 52],
            [70, 140, 190],
            [238, 239, 225]
        ],
        dtype=np.uint8
    )

    colored = palette[labels]

    picture = Image.fromarray(
        colored,
        mode="RGB"
    )

    buffer = io.BytesIO()

    picture.save(
        buffer,
        format="PNG"
    )

    return base64.b64encode(
        buffer.getvalue()
    ).decode("ascii")


# Read DICOM, PNG or JPG image
def load_image(path):

    path = Path(path)

    extension = path.suffix.lower()


    if extension == ".dcm":

        dataset = pydicom.dcmread(path)

        # Some DICOM files, such as test.dcm, only contain metadata
        if "PixelData" not in dataset:
            raise ValueError(
                "This DICOM contains metadata only and has no image pixels."
            )

        image = dataset.pixel_array

        modality = str(
            getattr(
                dataset,
                "Modality",
                "DICOM"
            )
        )


        # Use first frame if this is a multi-frame DICOM
        if (
            image.ndim == 3 and
            image.shape[-1] not in (3, 4)
        ):
            image = image[0]


        # Convert RGB DICOM to grayscale
        if (
            image.ndim == 3 and
            image.shape[-1] in (3, 4)
        ):
            image = np.mean(
                image[..., :3],
                axis=2
            )


        return (
            image.astype(np.float32),
            modality,
            "DICOM"
        )


    if extension in (
        ".png",
        ".jpg",
        ".jpeg"
    ):

        picture = Image.open(
            path
        ).convert("L")


        # Avoid running K-Means on a huge image
        picture.thumbnail(
            (1200, 1200)
        )


        image = np.asarray(
            picture,
            dtype=np.float32
        )


        return (
            image,
            "IMAGE",
            extension[1:].upper()
        )


    raise ValueError(
        "Unsupported image format."
    )


# Run K-Means on the image pixels
def analyse(path):

    image, modality, input_type = load_image(path)

    # K-Means receives one grayscale value for each pixel
    pixels = image.reshape(-1, 1)

    model = KMeans(
        n_clusters=3,
        random_state=0,
        n_init=10
    )


    # Large images only need a sample for training
    if len(pixels) > 200000:

        generator = np.random.default_rng(0)

        indexes = generator.choice(
            len(pixels),
            size=200000,
            replace=False
        )

        model.fit(
            pixels[indexes]
        )

        labels = model.predict(
            pixels
        )

    else:

        labels = model.fit_predict(
            pixels
        )


    centers = model.cluster_centers_[:, 0]


    # Order clusters from darkest to brightest
    order = np.argsort(
        centers
    )

    mapping = np.zeros(
        len(order),
        dtype=np.int32
    )


    for new_value, old_value in enumerate(order):

        mapping[old_value] = new_value


    labels = mapping[labels]

    sorted_centers = centers[order]

    clustered = labels.reshape(
        image.shape
    )

    counts = np.bincount(
        labels,
        minlength=3
    )


    # Images are returned as Base64 so React can display them directly
    result = {
        "source_name": Path(path).name,

        "input_type": input_type,

        "modality": modality,

        "width": int(
            image.shape[1]
        ),

        "height": int(
            image.shape[0]
        ),

        "min_pixel": float(
            np.min(image)
        ),

        "max_pixel": float(
            np.max(image)
        ),

        "cluster_centers": [
            float(value)
            for value in sorted_centers
        ],

        "counts": [
            int(value)
            for value in counts
        ],

        "original_image":
            png_base64(image),

        "clustered_image":
            clustered_png_base64(
                clustered
            )
    }


    return result


# Save result where the C++ server expects it
def write_result(result):

    OUTPUT_FILE.parent.mkdir(
        exist_ok=True
    )

    with open(
        OUTPUT_FILE,
        "w"
    ) as file:

        json.dump(
            result,
            file
        )


def main():

    try:

        # If C++ sends a filename, analyze that file
        if len(sys.argv) > 1:

            image_path = sys.argv[1]

        else:

            # Used by the "Run demo CT" button
            image_path = get_testdata_file(
                "CT_small.dcm"
            )


        result = analyse(
            image_path
        )


    except Exception as error:

        result = {
            "error": str(error)
        }


    write_result(
        result
    )


    print(
        json.dumps(
            result
        )
    )


if __name__ == "__main__":
    main()