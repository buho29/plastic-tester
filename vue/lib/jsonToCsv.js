// jsonToCsv.js

class JsonToCsv {
  static convert(json, name, date, description, count) {
    if (!Array.isArray(json) || json.length === 0) {
      throw new Error("El JSON debe ser un array no vacío.");
    }

    // Renombrar las claves del JSON
    const renamedJson = json.map((obj) => ({
      distance: obj.d,
      force: obj.f,
      time: obj.t,
      min: obj.mi,
      max: obj.ma,
    }));

    // Obtener las cabeceras del CSV
    const headers = Object.keys(renamedJson[0]);

    // Crear las filas del CSV
    const rows = renamedJson.map((obj) => {
      return headers
        .map((header) => {
          let value = obj[header];
          // Reemplazar el punto decimal por coma en números
          if (typeof value === "number") {
            value = value.toString().replace(".", ",");
          }
          // Escapar comillas y asegurarse de que los valores estén entre comillas si contienen tabuladores
          if (typeof value === "string" && value.includes("\t")) {
            value = `"${value.replace(/"/g, '""')}"`;
          }
          return value;
        })
        .join("\t"); // Usar tabulador como separador
    });

    // Crear las filas adicionales para nombre, fecha y descripción
    const metadataRows = [
      `Name:\t${name}`,
      `Date:\t${date}`,
      `Description:\t${description}`,
      `N° tests:\t${count}`,
      "", // Fila vacía
    ];

    // Unir todo: metadatos, fila vacía, cabeceras y filas de datos
    const csv = [
      ...metadataRows,
      headers.join("\t"),
      ...rows,
    ].join("\n");

    return csv;
  }

  static downloadDirect(csv, filename = "data.csv") {
    // Crear un Blob con el contenido CSV
    const blob = new Blob([csv], { type: "text/csv;charset=utf-8;" });

    // Crear un enlace temporal
    const link = document.createElement("a");
    link.href = URL.createObjectURL(blob);
    link.download = filename;
    link.style.visibility = "hidden";

    // Añadir el enlace al DOM y hacer clic en él
    document.body.appendChild(link);
    link.click();

    // Limpiar y eliminar el enlace
    document.body.removeChild(link);
    URL.revokeObjectURL(link.href);
  }

  static async saveCsvWithDialog(csv, filename = "data.csv") {
    try {
      // Verificar si la API de File System Access es compatible
      if (!window.showSaveFilePicker) {
        this.downloadDirect(csv, filename);
        return;
      }

      // Mostrar el diálogo para guardar el archivo
      const fileHandle = await window.showSaveFilePicker({
        suggestedName: filename,
        types: [
          {
            description: "Archivos CSV",
            accept: { "text/csv": [".csv"] },
          },
        ],
      });

      // Crear un escritor para el archivo
      const writable = await fileHandle.createWritable();

      // Escribir el contenido CSV en el archivo
      await writable.write(csv);
      await writable.close();
    } catch (error) {
      console.error("Error al guardar el archivo: ", error);
    }
  }

  static legacyCopyToClipboard(csv) {
    try {
      
      // Crear un elemento textarea temporal
      const textarea = document.createElement('textarea');
      textarea.value = csv;
      textarea.style.position = 'fixed';
      textarea.style.left = '-9999px';
      textarea.style.top = '-9999px';
      
      document.body.appendChild(textarea);
      textarea.select();
      
      // Intentar copiar
      const success = document.execCommand('copy');
      
      // Limpiar
      document.body.removeChild(textarea);
      
      if (!success) throw new Error('Falló el comando copy');
      
      console.log("CSV copiado al portapapeles con éxito (método legacy)");
      return true;
    } catch (error) {
      console.error("Error al copiar con método legacy:", error);
      return false;
    }
  }

  static async robustCopy(csv) {
    try {
      
      // Primero intentar con la API moderna
      if (navigator.clipboard) {
        await navigator.clipboard.writeText(csv);
        console.log("Copiado con Clipboard API");
        return true;
      }
      
      // Fallback a execCommand
      return this.legacyCopyToClipboard(csv);
    } catch (error) {
      console.error("Error en copiado robusto:", error);
      return false;
    }
  }
  

  static async copy(text) {
    return navigator.clipboard
      .writeText(text)
      .then(() => true)
      .catch((err) => {
        console.error("Error al copiar al portapapeles: ", err);
      });
  }

  static async saveWithDialog(json, name, date, description, count) {
    try {
      const filename = name + ".csv";
      const csv = this.convert(json, name, date, description, count);
      await this.saveCsvWithDialog(csv, filename);

      return true; 
    } catch (error) {
      return false; 
    }
  }

  static copyToClipboard(json, name, date, description, count) {
    try {
      const csv = this.convert(json, name, date, description,count);
      this.robustCopy(csv).then((success) => {
        if (success) {
          console.log("CSV copiado al portapapeles con éxito.");
        } else {
          console.log("No se pudo copiar el CSV al portapapeles.");
        }
      });
      return true;
    } catch (error) {
      console.error("Error al convertir JSON a CSV: ", error);
      return false;
    }
  }
}

// Ejemplo de uso
/*const json = [
    {"d":3.442,"f":0.21,"t":3682},
    {"d":3.547,"f":0.31,"t":3786},
    {"d":3.647,"f":1.46,"t":3887},
    {"d":3.752,"f":2.37,"t":3991},
    {"d":3.852,"f":3.31,"t":4092},
    {"d":3.956,"f":4.08,"t":4196},
    {"d":4.057,"f":4.73,"t":4297},
    {"d":4.161,"f":4.97,"t":4401},
    {"d":4.262,"f":4.83,"t":4502},
    {"d":4.366,"f":4.74,"t":4605},
    {"d":4.466,"f":2.49,"t":4706},
    {"d":4.571,"f":0.68,"t":4810},
    {"d":4.671,"f":0.05,"t":4911}
  ];
  
  JsonToCsv.saveWithDialog(json,'ejemplo','10/03/2025','description',0);
  JsonToCsv.copyToClipboard(json,'ejemplo','10/03/2025','description',0);
  
  */
