// mixin para insertar notificaciones
const mixinNotify = {
  methods: {
    notify: function (msg) {
      //quasar notify
      this.$q.notify({
        color: "primary",
        textColor: "white",
        icon: "icon-cloud_done",
        message: msg,
      });
    },
    notifyW: function (msg) {
      this.$q.notify({
        color: "accent",
        textColor: "dark",
        icon: "icon-warning",
        message: msg,
      });
    },
  },
};

//vue components
Vue.component("b-sensor", {
  props: ["prop", "value"],
  template: /*html*/ `
    <q-card class="text-subtitle1 q-ma-lg">
      <q-card-section class="q-pa-none text-primary" style="min-width:100px;">
        <div class="q-pa-sm">{{ prop }}</div>
        <q-separator />
      </q-card-section>
      <q-card-section>
        <div class="text-dark">{{ value }}</div>
      </q-card-section>
    </q-card>
  `,
});

Vue.component("b-container", {
  props: ["title"],
  template: /*html*/ `
  <q-card
    class="q-my-lg q-mx-auto text-center bg-white page"
  >
    <q-card-section class="q-pa-none q-ma-none">
      <div class="bg-primary text-white shadow-3 round-top">
        <div class="q-pa-sm text-h6 ">{{ title }}</div>
      </div>
    </q-card-section>
    <q-card-section class="q-pa-none">
      <slot></slot>
    </q-card-section>
  </q-card>
  `,
});

Vue.component("compare-chart", {
  data() {
    return {
      chartData: null,
      labels: { f: "Kg", d: "mm", t:"ms"},
      colors: { f: "#A6D5E8", d: "#B8E2C8",t:"#CAB8E2"},
      options: {
        scales: {
          y: {
            beginAtZero: true,
            grid: { display: true },
          },
          x: {
            beginAtZero: true,
            grid: { display: false },
          },
        },
        legend: { display: false },
        responsive: true,
        maintainAspectRatio: false,
        height: 200,
      },
    };
  },
  props: ["rawData", "prop"],
  extends: VueChartJs.Bar,
  mixins: [VueChartJs.mixins.reactiveData],
  methods: {
    update() {
      // Crear labels y datos
      const arrTime = this.rawData.map(result => result.name);
      const arrData = this.rawData.map(result => {
        const axisY = result.data.map(m => m[this.prop]);
        return axisY.length > 0 ? Math.max(...axisY) : 1;
      });

      // Crear datasets
      const datasets = [
        this.makeDataSet(this.labels[this.prop], 
          arrData, this.colors[this.prop])
      ];

      // Actualizar chartData
      this.chartData = {
        labels: arrTime,
        datasets: datasets,
      };
    },
    makeDataSet(label, data, color) {
      return {
        label: label,
        backgroundColor: color,
        pointBackgroundColor: "white",
        borderWidth: 1,
        pointBorderColor: "#249EBF",
        data: data,
      };
    },
  },
  watch: {
    rawData: function (newQuestion, oldQuestion) {
      this.update();
    },
    prop: function (newQuestion, oldQuestion) {
      this.update();
    },
  },
  mounted() {
    this.update();
    this.renderChart(this.chartData, this.options);
  },
});

Vue.component("result-chart", {
  extends: VueChartJs.Line,
  mixins: [VueChartJs.mixins.reactiveData],
  props: ["rawData"],
  data() {
    return {
      
      options: {
        responsive: true,
        maintainAspectRatio: false,
        scales: {
          x: {
            type: 'linear', position: 'bottom',
            title: { display: true, text: 'Time (ms)' },
          },
          y: {
            title: { display: true, text: 'Value' },
            beginAtZero: true
          }
        },
        plugins: {
          tooltip: {
            callbacks: {
              label: (context) => {
                const dataPoint = this.rawData[context.dataIndex];
                return `Force: ${dataPoint.f}kg, Distance: ${dataPoint.d}mm`;
              }
            }
          }
        }
      },

      chartData: {
        labels: [], // Tiempo (eje X)
        datasets: [],
      },
    };
  },
  mounted() {
    this.updateChart();
    this.renderChart(this.chartData, this.options);
  },
  methods: {
    makeDataSet(label, data, color) {
      return {
        label: label,
        backgroundColor: color,
        borderColor: color,
        borderWidth: 2,
        data: data,
      };
    },
    updateChart() 
    {
      // Crear labels y datos
      const arrTime  = this.rawData.map((item) => item.t); // Tiempo (eje X)
      const arrForce = this.rawData.map((item) => ({ x: item.t, y: item.f }));
      const arrDist = this.rawData.map((item) => ({ x: item.t, y: item.d }));
      
      const datasets = [
        this.makeDataSet("Force (kg)",arrForce,"#A6D5E8"),
        this.makeDataSet("Distance (mm)",arrDist,"#CAB8E2")
      ]

      // Actualizar chartData
      this.chartData = {
        labels: arrTime,
        datasets: datasets,
      };
      
    },
  },
  watch: {
    rawData: function (newQuestion, oldQuestion) 
    {
      this.updateChart();
      //console.log(JSON.stringify(this.rawData));
    },
  },
});
