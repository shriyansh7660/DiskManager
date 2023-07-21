import os
import random
import string

def generate_text_file(file_name, file_size):
    with open(file_name, 'w') as file:
        content = ''.join(random.choices(string.ascii_letters, k=file_size))
        file.write(content)

# Create 5 text files with random sizes between 1 MB and 10 MB
for i in range(1, 4):
    file_name = f'test_file.txt'
    file_size = random.randint(1, 10) * 1024 * 1024  # 1 MB to 10 MB
    generate_text_file(file_name, file_size)
    print(f'Created {file_name} with size {file_size} bytes.')
