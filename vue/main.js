//router
// cuando requiresAuth=true requiere autentificacion
const pagesRoute = [
  {
    name: "home",
    path: "/",
    component: pageHome,
    meta: { title: "Tester", requiresAuth: false },
  },
  {
    name: "history",
    path: "/history",
    component: pageHistory,
    meta: { title: "History", requiresAuth: false },
  },
  {
    name: "test",
    path: "/test",
    component: pageTest,
    meta: { title: "Run Test", requiresAuth: true },
  },
  {
    name: "move",
    path: "/move",
    component: pageMove,
    meta: { title: "Move", requiresAuth: true },
  },
  {
    name: "chart",
    path: "/result/:name",
    component: pageResult,
    meta: { title: "Result", requiresAuth: false },
  },
  {
    name: "login",
    path: "/login",
    component: pageLogin,
    meta: { title: "Login", requiresAuth: false },
  },
  {
    name: "options",
    path: "/options",
    component: pageOptions,
    meta: { title: "Options", requiresAuth: true },
  },
  {
    name: "system",
    path: "/system",
    component: pageSystem,
    meta: { title: "System", requiresAuth: true },
  },
];

//menu leftdata
const menu = [
  { title: "Home", icon: "icon-home", path: "/" },
  { title: "Run Test", icon: "icon-directions_run", path: "/test" },
  { title: "Move", icon: "icon-control_camera", path: "/move" },
  { title: "Historial", icon: "icon-trending_up", path: "/history" },
  { title: "Opciones", icon: "icon-settings", path: "/options" },
  { title: "System", icon: "icon-build", path: "/system" },
];

Vue.use(VueRouter);

const router = new VueRouter({ routes: pagesRoute });

//aplication
const app = new Vue({
  el: "#app",
  router,
  store,
  mixins: [mixinNotify],
  data: function () {
    return {
      position: 0,
      left: false,
      transitionName: "slide-right",
      pages: menu,
      showPopup: false,
      showButton: false,
      title: "",
      cmdString: "",
      showLoading: false,
      message: "",
    };
  },
  computed: {
    ...Vuex.mapState(["authenticate", "isConnected"]),
  },
  methods: {
    // importamos acciones
    ...Vuex.mapActions(["connect", "logoutUser", "sendCmd"]),
    ...Vuex.mapMutations(["unload"]),

    userAuthenticated() {
      return this.authenticate;
    },
    go(direction) {
      this.$router.go(direction);
    },
    onPress() {
      this.sendCmd(JSON.parse(this.cmdString));
    },
  },
  beforeCreate() {
    //cargamos localStorage (token)
    this.$store.commit("loadLocal");
  },
  mounted() {
    this.$q.iconSet = iconSet;
  },
  beforeDestroy() {
    this.unsubscribe();
  },
  created() {
    this.connect();
    // console.log('Updating mutation', action.type,action.payload);
    // nos registrammos a las llamadas de acciones en vuex
    // para interpretar notificaciones y acciones por parte del servidor
    this.unsubscribe = this.$store.subscribeAction((action, state) => {
      switch (action.type) {
        // notificaciones
        case "notify":
          let pl = action.payload;
          let msg = pl.content;
          //ok
          if (pl.type === 0) this.notify(msg);
          //error
          else if (pl.type === 1) {
            this.title = "Alert";
            this.message = msg;
            this.showButton = false;
            this.showPopup = true;
          } // warn
          else if (pl.type == 2) {
            this.notifyW(msg);
          }
          // warn + ok button
          else if (pl.type == 3) {
            let arr = msg.split("|");
            this.title = arr[0];
            this.message = arr[1];
            this.cmdString = arr[2];
            this.showButton = true;
            this.showPopup = true;
          }
          break;
        // acciones servidor
        case "goTo":
          let path = action.payload;
          if (this.$route.path !== path) {
            this.$router.push(path);
          }
          break;
        case "logout":
          userLogOut();
          break;
        // eventos websocket
        case "onClose":
          this.notifyW("disconnected");
          break;
        case "onOpen":
          this.notify("Connected");
          break;
        case "onError":
          this.notifyW("Error");
          break;
      }
    });
  },
  watch: {
    $route(to, from) {
      this.transitionName =
        this.transitionName === "slide-left" ? "slide-right" : "slide-left";
    },
    //mostrando o escondiendo un "loading..."
    isConnected: function (newValue, oldValue) {
      this.showLoading = !newValue;
    },
  },
}).$mount("#app");

router.beforeEach((to, from, next) => {
  if (to.meta.requiresAuth && !app.userAuthenticated()) {
    next("/login");
  } else {
    next();
  }
});
