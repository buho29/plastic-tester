import os
import shutil
import gzip
import shutil

# Comprimir archivos de vue y copiarlos a data/www

def comprimir_archivos(source_dir, target_dir):
    """Comprime recursivamente archivos y carpetas en un directorio, 
       respetando la estructura de directorios y guardando en gzip."""

    for root, dirs, files in os.walk(source_dir):
        for file in files:
            source_path = os.path.join(root, file)
            relative_path = os.path.relpath(source_path, source_dir)
            target_path = os.path.join(target_dir, relative_path + ".gz")

            # Crea el directorio destino si no existe
            os.makedirs(os.path.dirname(target_path), exist_ok=True)

            with open(source_path, 'rb') as f_in, gzip.open(target_path, 'wb') as f_out:
                shutil.copyfileobj(f_in, f_out)

def limpiar_directorio(directory):
    """Borra el contenido de un directorio."""
    for root, dirs, files in os.walk(directory):
        for file in files:
            file_path = os.path.join(root, file)
            try:
                os.remove(file_path)
            except OSError as e:
                print(f"No se pudo borrar {file_path}. Razón: {e}")
        for dir in dirs:
            dir_path = os.path.join(root, dir)
            try:
                shutil.rmtree(dir_path)
            except OSError as e:
                print(f"No se pudo borrar {dir_path}. Razón: {e}")

# Directorios
source_dir = "vue"  # Carpeta con los archivos originales
target_dir = "data/www" # Carpeta destino

# Limpiar directorio destino
print(f"Limpiando {target_dir}...")
limpiar_directorio(target_dir)

# Comprimir archivos
print(f"Comprimiendo archivos desde {source_dir} a {target_dir}...")
comprimir_archivos(source_dir, target_dir)

print("Compresión y copia completadas.")