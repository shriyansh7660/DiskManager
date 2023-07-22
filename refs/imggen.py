import os
import random
import requests
from urllib.parse import urlparse

def download_random_image(output_folder, num_images=10):
    if not os.path.exists(output_folder):
        os.makedirs(output_folder)

    for i in range(num_images):
        try:
            url = get_random_image_url()
            response = requests.get(url)

            if response.status_code == 200:
                image_name = os.path.join(output_folder, f"image_{i+1}.jpg")
                with open(image_name, "wb") as f:
                    f.write(response.content)
                print(f"Image {i+1} downloaded successfully.")
            else:
                print(f"Failed to download image {i+1}, status code: {response.status_code}")
        except Exception as e:
            print(f"Error while downloading image {i+1}: {e}")

def get_random_image_url():
    image_sources = [
        "https://source.unsplash.com/random",
        "https://picsum.photos/800/600",
        # Add more image sources as needed...
    ]
    return random.choice(image_sources)

if __name__ == "__main__":
    output_folder = "random_images"
    num_images = 10  # Change this value to specify the number of images to download
    download_random_image(output_folder, num_images)
