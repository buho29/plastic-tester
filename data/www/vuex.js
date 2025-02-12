//model
Vue.use(Vuex);
const store = new Vuex.Store({
  state: {
    sensors: {
      d: 0,
      f: 0,
    },
    history: [],
    results: [],
    lastResult: [],
    config: {},
    system: {},
    isConnected: false,
    authenticate: false,
  },
  mutations: {
    // update state

    updateSensors(state, obj) {
      state.sensors = obj;
    },
    updateConfig(state, obj) {
      state.config = obj;
    },
    updateSystem(state, obj) {
      state.system = obj;
    },
    updateHistory(state, obj) {
      state.history = obj;
    },
    updateResults(state, obj) {
      state.results = obj;
    },
    updateLastResult(state, obj) {
      state.lastResult = obj;
    },

    //login
    authenticated(state, token) {
      state.authenticate = true;
      state.token = token;
      localStorage.setItem("token", token);
    },
    //logout
    logout(state) {
      state.authenticate = false;
      state.token = undefined;
      localStorage.removeItem("token");
    },
    //carga local
    loadLocal(state) {
      let token = localStorage.getItem("token");
      if (token) {
        state.authenticate = true;
        state.token = token;
      }
    },
    //se cierra la ventana
    unload(state) {
      if (this.connection !== undefined) {
        this.connection.close();
      }
    },
    //conected
    connected(state, bool) {
      state.isConnected = bool;
    } /**/,
  },
  actions: {
    // connexion inicial websocket
    connect({ commit, dispatch }) {
      if (this.connection !== undefined) {
        this.connection.close();
      }

      console.log(host);

      this.connection = new WebSocket("ws://" + host + "/ws");

      //delegamos los eventos a las acciones
      this.connection.onmessage = (event) => dispatch("onMessage", event);
      this.connection.onclose = (event) => dispatch("onClose", event);
      this.connection.onerror = (event) => dispatch("onError", event);
      this.connection.onopen = (event) => dispatch("onOpen", event);
    },
    //reiniciar esp32
    restart({ dispatch }) {
      dispatch("send", {
        cmd: { restar: 1 },
      });
    },
    // intento de autentificacion
    setAuthentication({ dispatch, commit, state }, user) {
      if (this.connection !== undefined) {
        this.connection.send(JSON.stringify({ login: user }));
      } else dispatch("connect");
    },
    // recibe wasaps del servidor de eventos/errores
    // hay q suscribirse a vuex para tratar los mensajes
    notify({ commit, state }, notify) {
      //is event
    },
    // para manejar el routing desde servidor y vuex
    // podemos abrir paginas al usuario
    // hay q suscribirse a vuex
    goTo({ commit, state }, path) {
      //is event
    },
    // para cerrar la session del usuario desde el servidor
    logoutUser({ dispatch, commit, state }, path) {
      commit("logout");
      dispatch("goTo", "/login");
    },
    /***********************
     *      data
     ***********************/
    //para descargar datos publicos
    // [ "/result/pla.json", "/result/abs.json" ]
    loadResults({ dispatch }, indexes) {
      dispatch("send", { load: { indexes: indexes } });
    },

    //para descargar datos autenticadas
    //pageId = 0 option | pageId = 1 system
    loadAuth({ dispatch }, pageId) {
      dispatch("send", { loadAuth: pageId });
    },

    editConfig({ commit, dispatch }, obj) {
      dispatch("send", { config: obj });
    },

    sendCmd({ commit, dispatch }, obj) {
      dispatch("send", { cmd: obj });
    },

    //envia objectos/comandos en json al servidor
    send({ commit, dispatch, state }, obj) {
      // si estamos identificado
      if (state.authenticate) {
        //metemos el token
        obj.token = state.token;
      }

      if (this.connection !== undefined) {
        this.connection.send(JSON.stringify(obj));
      } else dispatch("connect");
    },
    //privado
    // eventos websocket
    //json recibido del Esp32
    onMessage({ commit, dispatch }, event) {
      //convierte json en un objecto js
      let json = JSON.parse(event.data);
      // TODO mutations
      const mutations = {
        ss: "updateSensors",
        token: "authenticated",
        system: "updateSystem",
        config: "updateConfig",
        history: "updateHistory",
        results: "updateResults",
        lastResult: "updateLastResult",
      };
      //actualizamos los datos por commit("mutation")
      for (const key in mutations) {
        if (json[key] !== undefined) 
          commit(mutations[key], json[key]);
      }

      const actions = {
        message: "notify",
        goTo: "goTo",
        logout: "logoutUser",
      };
      //ejecutamos acciones dispatch("action")
      for (const key in actions) {
        if (json[key] !== undefined) dispatch(actions[key], json[key]);
      }
    },
    onClose({ commit, dispatch }, event) {
      // ponemos state.isConnected a false
      commit("connected", false);
      //reconectamos al pasar 3sg
      setTimeout(() => {
        dispatch("connect");
      }, 3000);
    },
    onError({ commit, state }, event) {
      //is event
    },
    onOpen({ commit, state }, event) {
      commit("connected", true);
    } /**/,
  },
  getters: {
    getHistoryIndex: (state) => (name) => {
      return state.history.findIndex((item) => item.name === name);
    },
    // Obtener la fecha y hora actual
    getCurrentDate: (state) => () => {
      const now = new Date();

      // Extraer día, mes, año, horas y minutos
      const d = String(now.getDate()).padStart(2, "0"); // Día con dos dígitos
      const m = String(now.getMonth() + 1).padStart(2, "0"); // Mes con dos dígitos (los meses van de 0 a 11)
      const y = now.getFullYear(); // Año con cuatro dígitos
      const h = String(now.getHours()).padStart(2, "0"); // Horas con dos dígitos
      const min = String(now.getMinutes()).padStart(2, "0"); // Minutos con dos dígitos

      // Formatear la fecha y hora en el formato deseado
      return `${d}/${m}/${y} ${h}:${min}`;
    },
  },
  strict: true,
});
