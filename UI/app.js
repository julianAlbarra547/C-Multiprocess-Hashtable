const SAMPLE_DATA_PATH = "./data/spotify_sample.csv";
const MAX_VISIBLE_ROWS = 250;

const state = {
  rows: [],
  columns: [],
  filteredRows: [],
  selectedRowIndex: null,
  datasetName: "",
};

const elements = {
  loadSampleBtn: document.querySelector("#loadSampleBtn"),
  fileInput: document.querySelector("#fileInput"),
  searchInput: document.querySelector("#searchInput"),
  filterColumn: document.querySelector("#filterColumn"),
  filterValue: document.querySelector("#filterValue"),
  sortColumn: document.querySelector("#sortColumn"),
  sortDirection: document.querySelector("#sortDirection"),
  resetBtn: document.querySelector("#resetBtn"),
  statusText: document.querySelector("#statusText"),
  statsList: document.querySelector("#statsList"),
  tableSummary: document.querySelector("#tableSummary"),
  tableHead: document.querySelector("#dataTable thead"),
  tableBody: document.querySelector("#dataTable tbody"),
  detailsList: document.querySelector("#detailsList"),
  detailsEmpty: document.querySelector("#detailsEmpty"),
};

function parseCSV(csvText) {
  const rows = [];
  let currentRow = [];
  let currentValue = "";
  let insideQuotes = false;

  for (let i = 0; i < csvText.length; i += 1) {
    const char = csvText[i];
    const nextChar = csvText[i + 1];

    if (char === '"') {
      if (insideQuotes && nextChar === '"') {
        currentValue += '"';
        i += 1;
      } else {
        insideQuotes = !insideQuotes;
      }
      continue;
    }

    if (char === "," && !insideQuotes) {
      currentRow.push(currentValue);
      currentValue = "";
      continue;
    }

    if ((char === "\n" || char === "\r") && !insideQuotes) {
      if (char === "\r" && nextChar === "\n") {
        i += 1;
      }
      currentRow.push(currentValue);
      rows.push(currentRow);
      currentRow = [];
      currentValue = "";
      continue;
    }

    currentValue += char;
  }

  if (currentValue.length > 0 || currentRow.length > 0) {
    currentRow.push(currentValue);
    rows.push(currentRow);
  }

  return rows.filter((row) => row.some((value) => value.trim() !== ""));
}

function sanitizeColumns(headerRow) {
  return headerRow.map((column, index) => {
    const trimmed = column.trim();
    if (trimmed !== "") {
      return trimmed;
    }
    return index === 0 ? "id" : `column_${index + 1}`;
  });
}

function normalizeValue(value, column) {
  const trimmed = value.trim();
  if (trimmed === "") {
    return "";
  }

  if (["rank", "streams", "duration_ms", "id"].includes(column)) {
    const number = Number(trimmed);
    return Number.isNaN(number) ? trimmed : number;
  }

  if (column === "explicit") {
    return trimmed.toLowerCase() === "true";
  }

  return trimmed;
}

function rowsToObjects(parsedRows) {
  const [headerRow, ...dataRows] = parsedRows;
  const columns = sanitizeColumns(headerRow);
  const rows = dataRows.map((row, rowIndex) => {
    const record = {};
    columns.forEach((column, columnIndex) => {
      record[column] = normalizeValue(row[columnIndex] ?? "", column);
    });
    if (record.id === "") {
      record.id = rowIndex;
    }
    return record;
  });

  return { columns, rows };
}

function populateSelect(select, columns, defaultLabel) {
  const previous = select.value;
  select.innerHTML = "";

  const blankOption = document.createElement("option");
  blankOption.value = "";
  blankOption.textContent = defaultLabel;
  select.appendChild(blankOption);

  columns.forEach((column) => {
    const option = document.createElement("option");
    option.value = column;
    option.textContent = column;
    select.appendChild(option);
  });

  if (columns.includes(previous)) {
    select.value = previous;
  }
}

function loadDataset(rows, columns, datasetName) {
  state.rows = rows;
  state.columns = columns;
  state.datasetName = datasetName;
  state.selectedRowIndex = null;

  populateSelect(elements.filterColumn, columns, "Todas las columnas");
  populateSelect(elements.sortColumn, columns, "Sin orden");
  applyFilters();
}

function applyFilters() {
  const searchTerm = elements.searchInput.value.trim().toLowerCase();
  const filterColumn = elements.filterColumn.value;
  const filterValue = elements.filterValue.value.trim().toLowerCase();
  const sortColumn = elements.sortColumn.value;
  const sortDirection = elements.sortDirection.value;

  let result = [...state.rows];

  if (searchTerm) {
    result = result.filter((row) =>
      state.columns.some((column) =>
        String(row[column]).toLowerCase().includes(searchTerm),
      ),
    );
  }

  if (filterColumn && filterValue) {
    result = result.filter((row) =>
      String(row[filterColumn]).toLowerCase().includes(filterValue),
    );
  }

  if (sortColumn) {
    result.sort((left, right) => {
      const a = left[sortColumn];
      const b = right[sortColumn];
      let comparison = 0;

      if (typeof a === "number" && typeof b === "number") {
        comparison = a - b;
      } else {
        comparison = String(a).localeCompare(String(b));
      }

      return sortDirection === "desc" ? -comparison : comparison;
    });
  }

  state.filteredRows = result;
  render();
}

function renderStats() {
  const totalRows = state.rows.length;
  const visibleRows = state.filteredRows.length;
  const explicitCount = state.rows.filter((row) => row.explicit === true).length;
  const uniqueArtists = new Set(state.rows.map((row) => row.artist)).size;

  elements.statusText.textContent = totalRows
    ? `Dataset cargado: ${state.datasetName}`
    : "No hay ningun dataset cargado.";

  elements.statsList.innerHTML = "";
  if (!totalRows) {
    return;
  }

  const stats = [
    ["Filas", totalRows.toLocaleString()],
    ["Columnas", state.columns.length.toString()],
    ["Visibles", visibleRows.toLocaleString()],
    ["Artistas unicos", uniqueArtists.toLocaleString()],
    ["Canciones explicitas", explicitCount.toLocaleString()],
  ];

  stats.forEach(([label, value]) => {
    const dt = document.createElement("dt");
    dt.textContent = label;
    const dd = document.createElement("dd");
    dd.textContent = value;
    elements.statsList.append(dt, dd);
  });
}

function renderTable() {
  elements.tableHead.innerHTML = "";
  elements.tableBody.innerHTML = "";

  if (!state.columns.length) {
    elements.tableSummary.textContent = "Carga un CSV para comenzar.";
    return;
  }

  const headerRow = document.createElement("tr");
  state.columns.forEach((column) => {
    const th = document.createElement("th");
    th.textContent = column;
    headerRow.appendChild(th);
  });
  elements.tableHead.appendChild(headerRow);

  const rowsToRender = state.filteredRows.slice(0, MAX_VISIBLE_ROWS);
  elements.tableSummary.textContent =
    state.filteredRows.length > MAX_VISIBLE_ROWS
      ? `Mostrando las primeras ${MAX_VISIBLE_ROWS} de ${state.filteredRows.length.toLocaleString()} filas coincidentes.`
      : `Mostrando ${state.filteredRows.length.toLocaleString()} filas coincidentes.`;

  if (rowsToRender.length === 0) {
    const emptyRow = document.createElement("tr");
    const emptyCell = document.createElement("td");
    emptyCell.colSpan = state.columns.length;
    emptyCell.textContent = "Ninguna fila coincide con los filtros actuales.";
    emptyRow.appendChild(emptyCell);
    elements.tableBody.appendChild(emptyRow);
    return;
  }

  rowsToRender.forEach((row, index) => {
    const tr = document.createElement("tr");
    if (state.selectedRowIndex === index) {
      tr.classList.add("selected");
    }

    tr.addEventListener("click", () => {
      state.selectedRowIndex = index;
      renderDetails(row);
      renderTable();
    });

    state.columns.forEach((column) => {
      const td = document.createElement("td");
      td.textContent = String(row[column]);
      tr.appendChild(td);
    });

    elements.tableBody.appendChild(tr);
  });
}

function renderDetails(row) {
  if (!row) {
    elements.detailsEmpty.hidden = false;
    elements.detailsList.innerHTML = "";
    return;
  }

  elements.detailsEmpty.hidden = true;
  elements.detailsList.innerHTML = "";

  state.columns.forEach((column) => {
    const dt = document.createElement("dt");
    dt.textContent = column;
    const dd = document.createElement("dd");

    if (column === "url" && row[column]) {
      const link = document.createElement("a");
      link.href = row[column];
      link.textContent = row[column];
      link.target = "_blank";
      link.rel = "noreferrer";
      dd.appendChild(link);
    } else {
      dd.textContent = String(row[column]);
    }

    elements.detailsList.append(dt, dd);
  });
}

function render() {
  renderStats();
  renderTable();

  if (state.selectedRowIndex === null) {
    renderDetails(null);
    return;
  }

  const selected = state.filteredRows[state.selectedRowIndex];
  renderDetails(selected ?? null);
}

async function handleTextDataset(csvText, datasetName) {
  const parsedRows = parseCSV(csvText);
  if (parsedRows.length < 2) {
    throw new Error("El CSV debe incluir una fila de encabezado y al menos una fila de datos.");
  }

  const { rows, columns } = rowsToObjects(parsedRows);
  loadDataset(rows, columns, datasetName);
}

async function loadSampleData() {
  const response = await fetch(SAMPLE_DATA_PATH);
  if (!response.ok) {
    throw new Error(`No se pudo cargar el archivo de ejemplo: ${response.status}`);
  }

  const csvText = await response.text();
  await handleTextDataset(csvText, "dataset de ejemplo de Spotify");
}

function resetControls() {
  elements.searchInput.value = "";
  elements.filterColumn.value = "";
  elements.filterValue.value = "";
  elements.sortColumn.value = "";
  elements.sortDirection.value = "asc";
  state.selectedRowIndex = null;
  applyFilters();
}

elements.loadSampleBtn.addEventListener("click", async () => {
  try {
    await loadSampleData();
  } catch (error) {
    elements.statusText.textContent = error.message;
  }
});

elements.fileInput.addEventListener("change", async (event) => {
  const [file] = event.target.files;
  if (!file) {
    return;
  }

  try {
    const csvText = await file.text();
    await handleTextDataset(csvText, file.name);
  } catch (error) {
    elements.statusText.textContent = error.message;
  }
});

[
  elements.searchInput,
  elements.filterColumn,
  elements.filterValue,
  elements.sortColumn,
  elements.sortDirection,
].forEach((element) => {
  element.addEventListener("input", applyFilters);
  element.addEventListener("change", applyFilters);
});

elements.resetBtn.addEventListener("click", resetControls);

render();
